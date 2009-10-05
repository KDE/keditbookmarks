/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/
#include "dolphinsearchbox.h"

#include <config-nepomuk.h>

#include <kdialog.h>
#include <kglobalsettings.h>
#include <klineedit.h>
#include <klocale.h>
#include <kiconloader.h>

#include <QEvent>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QStandardItemModel>
#include <QtGui/QCompleter>
#include <QtGui/QTreeView>
#include <QToolButton>

#ifdef HAVE_NEPOMUK
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Tag>
#endif //HAVE_NEPOMUK


DolphinSearchCompleter::DolphinSearchCompleter(KLineEdit* linedit) :
    QObject(0),
    q(linedit),
    m_completer(0),
    m_completionModel(0),
    m_wordStart(-1),
    m_wordEnd(-1)
{
}

void DolphinSearchCompleter::init()
{
    m_completionModel = new QStandardItemModel(this);

#ifdef HAVE_NEPOMUK
    if (!Nepomuk::ResourceManager::instance()->init()) {
        //read all currently set tags
        //NOTE if the user changes tags elsewhere they won't get updated here
        QList<Nepomuk::Tag> tags = Nepomuk::Tag::allTags();
        foreach (const Nepomuk::Tag& tag, tags) {
            const QString tagText = tag.label();
            addCompletionItem(tagText,
                                "tag:\"" + tagText + '\"',
                                i18nc("Tag as in Nepomuk::Tag", "Tag"),
                                KIcon("mail-tagged"));
        }
    }
#endif //HAVE_NEPOMUK

    //add "and", "or" and "-" (not) logic operators
    addCompletionItem(i18nc("and ss in a logic operator to connect search terms", "and"),
                        "and",
                        "logic operator and");
    addCompletionItem(i18nc("or as in a logic operator to connect search terms", "or"),
                        "or",
                        "logic operator or");
    addCompletionItem(i18nc("not as in a logic operator to connect search terms", "not"),
                        "-",
                        "logic operator not");

    m_completionModel->sort(0, Qt::AscendingOrder);

    m_completer = new QCompleter(m_completionModel, this);
    m_completer->setWidget(q);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    QTreeView *view = new QTreeView;
    m_completer->setPopup(view);
    view->setRootIsDecorated(false);
    view->setHeaderHidden(true);

    connect(q, SIGNAL(textEdited(QString)), this, SLOT(slotTextEdited(QString)));
    connect(m_completer, SIGNAL(activated(QModelIndex)), this, SLOT(completionActivated(QModelIndex)));
    connect(m_completer, SIGNAL(highlighted(QModelIndex)), this, SLOT(highlighted(QModelIndex)));
}

void DolphinSearchCompleter::addCompletionItem(const QString& displayed, const QString& usedForCompletition, const QString& description, const KIcon& icon)
{
    QList<QStandardItem*> items;
    QStandardItem *item = new QStandardItem();
    item->setData(QVariant(displayed), Qt::DisplayRole);
    item->setData(QVariant(usedForCompletition), Qt::UserRole);
    items << item;

    item = new QStandardItem(description);
    item->setIcon(icon);
    items << item;

    m_completionModel->insertRow(m_completionModel->rowCount(), items);
}

void DolphinSearchCompleter::findText(int* wordStart, int* wordEnd, QString* newWord, int cursorPos, const QString &input)
{
    --cursorPos;//decrease to get a useful position (not the end of the word e.g.)

    if (!wordStart || !wordEnd) {
        return;
    }

    *wordStart = -1;
    *wordEnd = -1;

    //the word might contain "" and thus maybe spaces
    if (input.contains('\"')) {
        int tempStart = -1;
        int tempEnd = -1;

        do {
            tempStart = input.indexOf('\"', tempEnd + 1);
            tempEnd = input.indexOf('\"', tempStart + 1);
            if ((cursorPos >= tempStart) && (cursorPos <= tempEnd)) {
                *wordStart = tempStart;
                *wordEnd = tempEnd;
                break;
            } else if ((tempEnd == -1) && (cursorPos >= tempStart)) {
                //one " found, so probably the beginning of the new word
                *wordStart = tempStart;
                break;
            }
        } while ((tempStart != -1) && (tempEnd != -1));
    }

    if (*wordEnd > -1) {
        *wordEnd = input.indexOf(' ', *wordEnd) - 1;
    } else {
        *wordEnd = input.indexOf(' ', cursorPos) - 1;
    }
    if (*wordEnd < 0) {
        *wordEnd = input.length() - 1;
    }

    if (*wordStart > -1) {
        *wordStart = input.lastIndexOf(' ', *wordStart + 1) + 1;
    } else {
        *wordStart = input.lastIndexOf(' ', cursorPos) + 1;
    }
    if (*wordStart < 0) {
        *wordStart = 0;
    }


    QString word = input.mid(*wordStart, *wordEnd - *wordStart + 1);

    //remove opening braces or negations ('-' = not) at the beginning
    while (word.count() && ((word[0] == '(') || (word[0] == '-'))) {
        word.remove(0, 1);
        ++(*wordStart);
    }

    //remove ending braces at the end
    while (word.count() && (word[word.count() - 1] == ')')) {
        word.remove(word.count() - 1, 1);
        --(*wordEnd);
    }

    if (newWord) {
        *newWord = word;
    }
}

void DolphinSearchCompleter::slotTextEdited(const QString& text)
{
    findText(&m_wordStart, &m_wordEnd, &m_userText, q->cursorPosition(), text);

    if (!m_userText.isEmpty()) {
        const int role = m_completer->completionRole();

        //change the role used for comparison depending on what the user entered
        if (m_userText.contains(':') || m_userText.contains('\"')) {
            //assume that m_userText contains searchinformation like 'tag:"..."'
            if (role != Qt::UserRole) {
                m_completer->setCompletionRole(Qt::UserRole);
            }
        } else if (role != Qt::EditRole) {
            m_completer->setCompletionRole(Qt::EditRole);
        }

        m_completer->setCompletionPrefix(m_userText);
        m_completer->complete();
    }
}

void DolphinSearchCompleter::highlighted(const QModelIndex& index)
{
    QString text = q->text();
    int wordStart;
    int wordEnd;

    findText(&wordStart, &wordEnd, 0, q->cursorPosition(), text);

    QString replace = index.sibling(index.row(), 0).data(Qt::UserRole).toString();
    //show the originally entered text
    if (replace.isEmpty()) {
        replace = m_userText;
    }

    text.replace(wordStart, wordEnd - wordStart + 1, replace);
    q->setText(text);
    q->setCursorPosition(wordStart + replace.length());
}

void DolphinSearchCompleter::activated(const QModelIndex& index)
{
    if ((m_wordStart == -1) || (m_wordStart == -1)) {
        return;
    }

    const QString replace = index.sibling(index.row(), 0).data(Qt::UserRole).toString();
    QString newText = q->text();
    newText.replace(m_wordStart, m_wordEnd - m_wordStart + 1, replace);
    q->setText(newText);
}

DolphinSearchBox::DolphinSearchBox(QWidget* parent) :
    QWidget(parent),
    m_searchInput(0),
    m_searchButton(0),
    m_completer(0)
{
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);
    hLayout->setSpacing(0);

    m_searchInput = new KLineEdit(this);
    m_searchInput->setClearButtonShown(true);
    m_searchInput->setMinimumWidth(150);
    m_searchInput->setClickMessage(i18nc("@label:textbox", "Search..."));
    hLayout->addWidget(m_searchInput);
    connect(m_searchInput, SIGNAL(returnPressed()),
            this, SLOT(emitSearchSignal()));

    m_completer = new DolphinSearchCompleter(m_searchInput);
    m_completer->init();

    m_searchButton = new QToolButton(this);
    m_searchButton->setAutoRaise(true);
    m_searchButton->setIcon(KIcon("edit-find"));
    m_searchButton->setToolTip(i18nc("@info:tooltip", "Click to begin the search"));
    hLayout->addWidget(m_searchButton);
    connect(m_searchButton, SIGNAL(clicked()),
            this, SLOT(emitSearchSignal()));
}

DolphinSearchBox::~DolphinSearchBox()
{
    delete m_completer;
}

bool DolphinSearchBox::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        m_searchInput->setFont(KGlobalSettings::generalFont());
    } else if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            m_searchInput->clear();
        }
    }
    return QWidget::event(event);
}

void DolphinSearchBox::emitSearchSignal()
{
    emit search(KUrl("nepomuksearch:/" + m_searchInput->text()));
}

#include "dolphinsearchbox.moc"
