/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "renamedialog.h"

#include <klineedit.h>
#include <klocale.h>

#include <QLabel>
#include <QVBoxLayout>

RenameDialog::RenameDialog(const KUrl::List& items) :
    KDialog(),
    m_renameOneItem(false)
{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(320, minSize.height()));

    const int itemCount = items.count();
    Q_ASSERT(itemCount >= 1);
    m_renameOneItem = (itemCount == 1);

    setCaption(m_renameOneItem ? i18n("Rename Item") : i18n("Rename Items"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    setButtonGuiItem(Ok, KGuiItem(i18n("Rename"), "dialog-apply"));

    QWidget* page = new QWidget(this);
    setMainWidget(page);

    QVBoxLayout* topLayout = new QVBoxLayout(page);
    topLayout->setMargin(KDialog::marginHint());

    QLabel* editLabel = 0;
    if (m_renameOneItem) {
        const KUrl& url = items.first();
        m_newName = url.fileName();
        editLabel = new QLabel(i18n("Rename the item '%1' to:", m_newName),
                               page);
    } else {
        m_newName = i18n("New name #");
        editLabel = new QLabel(i18n("Rename the %1 selected items to:", itemCount),
                               page);
    }

    m_lineEdit = new KLineEdit(page);
    QString postfix(items[0].prettyUrl().section('.', 1));
    if (postfix.length() > 0) {
        // The first item seems to have a postfix (e. g. 'jpg' or 'txt'). Now
        // check whether all other items have the same postfix. If this is the
        // case, add this postfix to the name suggestion.
        postfix.insert(0, '.');
        for (int i = 1; i < itemCount; ++i) {
            if (!items[i].prettyUrl().contains(postfix)) {
                // at least one item does not have the same postfix
                postfix.truncate(0);
                break;
            }
        }
    }

    int selectionLength = m_newName.length();
    if (!m_renameOneItem) {
        --selectionLength; // don't select the # character
    }

    const int postfixLength = postfix.length();
    if (postfixLength > 0) {
        if (m_renameOneItem) {
            selectionLength -= postfixLength;
        } else {
            m_newName.append(postfix);
        }
    }

    m_lineEdit->setText(m_newName);
    m_lineEdit->setSelection(0, selectionLength);
    m_lineEdit->setFocus();

    topLayout->addWidget(editLabel);
    topLayout->addWidget(m_lineEdit);

    if (!m_renameOneItem) {
        QLabel* infoLabel = new QLabel(i18n("(# will be replaced by ascending numbers)"), page);
        topLayout->addWidget(infoLabel);
    }
}

RenameDialog::~RenameDialog()
{}

void RenameDialog::slotButtonClicked(int button)
{
    if (button == Ok) {
        m_newName = m_lineEdit->text();
        if (m_newName.isEmpty()) {
            m_errorString = i18n("The new name is empty. A name with at least one character must be entered.");
        } else if (!m_renameOneItem && m_newName.contains('#') != 1) {
            m_newName.truncate(0);
            m_errorString = i18n("The name must contain exactly one # character.");
        }
    }

    KDialog::slotButtonClicked(button);
}

#include "renamedialog.moc"
