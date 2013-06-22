/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz (peter.penz@gmx.at)             *
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

#include <KLineEdit>
#include <KLocale>
#include <konq_operations.h>
#include <KStringHandler>
#include <knuminput.h>
#include <kmimetype.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

/**
 * Helper function for sorting items with qSort() in
 * DolphinView::renameSelectedItems().
 */
bool lessThan(const KFileItem& item1, const KFileItem& item2)
{
    return KStringHandler::naturalCompare(item1.name(), item2.name()) < 0;
}

RenameDialog::RenameDialog(QWidget *parent, const KFileItemList& items) :
    KDialog(parent),
    m_renameOneItem(false),
    m_newName(),
    m_lineEdit(0),
    m_items(items),
    m_allExtensionsDifferent(true),
    m_spinBox(0)
{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(320, minSize.height()));

    const int itemCount = items.count();
    Q_ASSERT(itemCount >= 1);
    m_renameOneItem = (itemCount == 1);

    setCaption(m_renameOneItem ?
               i18nc("@title:window", "Rename Item") :
               i18nc("@title:window", "Rename Items"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    setButtonGuiItem(Ok, KGuiItem(i18nc("@action:button", "&Rename"), "dialog-ok-apply"));

    QWidget* page = new QWidget(this);
    setMainWidget(page);

    QVBoxLayout* topLayout = new QVBoxLayout(page);

    QLabel* editLabel = 0;
    if (m_renameOneItem) {
        m_newName = items.first().name();
        editLabel = new QLabel(i18nc("@label:textbox", "Rename the item <filename>%1</filename> to:", m_newName),
                               page);
        editLabel->setTextFormat(Qt::PlainText);
    } else {
        m_newName = i18nc("@info:status", "New name #");
        editLabel = new QLabel(i18ncp("@label:textbox",
                                      "Rename the %1 selected item to:",
                                      "Rename the %1 selected items to:", itemCount),
                               page);
    }

    m_lineEdit = new KLineEdit(page);
    connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotTextChanged(QString)));

    int selectionLength = m_newName.length();
    if (m_renameOneItem) {
        const QString fileName = items.first().url().prettyUrl();
        const QString extension = KMimeType::extractKnownExtension(fileName.toLower());

        // If the current item is a directory, select the whole file name.
        if ((extension.length() > 0) && !items.first().isDir()) {
            // Don't select the extension
            selectionLength -= extension.length() + 1;
        }
    } else {
         // Don't select the # character
        --selectionLength;
    }

    m_lineEdit->setText(m_newName);
    m_lineEdit->setSelection(0, selectionLength);
    m_lineEdit->setFocus();

    topLayout->addWidget(editLabel);
    topLayout->addWidget(m_lineEdit);

    if (!m_renameOneItem) {
        QSet<QString> extensions;
        foreach (const KFileItem& item, m_items) {
            const QString extension = KMimeType::extractKnownExtension(item.url().prettyUrl().toLower());

            if (extensions.contains(extension)) {
                m_allExtensionsDifferent = false;
                break;
            }

            extensions.insert(extension);
        }

        QLabel* infoLabel = new QLabel(i18nc("@info", "# will be replaced by ascending numbers starting with:"), page);
        m_spinBox = new KIntSpinBox(0, 10000, 1, 1, page, 10);

        QHBoxLayout* horizontalLayout = new QHBoxLayout(page);
        horizontalLayout->setMargin(0);
        horizontalLayout->addWidget(infoLabel);
        horizontalLayout->addWidget(m_spinBox);

        topLayout->addLayout(horizontalLayout);
    }
}

RenameDialog::~RenameDialog()
{
}

void RenameDialog::slotButtonClicked(int button)
{
    if (button == KDialog::Ok) {
        m_newName = m_lineEdit->text();

        if (m_renameOneItem) {
            Q_ASSERT(m_items.count() == 1);
            const KUrl oldUrl = m_items.first().url();
            KUrl newUrl = oldUrl;
            newUrl.setFileName(KIO::encodeFileName(m_newName));

            QWidget* widget = parentWidget();
            if (!widget) {
                widget = this;
            }

            KonqOperations::rename(widget, oldUrl, newUrl);
        } else {
            renameItems();
        }
    }

    KDialog::slotButtonClicked(button);
}

void RenameDialog::slotTextChanged(const QString& newName)
{
    bool enable = !newName.isEmpty() && (newName != QLatin1String("..")) && (newName != QLatin1String("."));
    if (enable && !m_renameOneItem) {
        const int count = newName.count(QLatin1Char('#'));
        if (count == 0) {
            // Renaming multiple files without '#' will only work if all extensions are different.
            enable = m_allExtensionsDifferent;
        } else {
            // Assure that the new name contains exactly one # (or a connected sequence of #'s)
            const int first = newName.indexOf(QLatin1Char('#'));
            const int last = newName.lastIndexOf(QLatin1Char('#'));
            enable = (last - first + 1 == count);
        }
    }
    enableButtonOk(enable);
}

void RenameDialog::renameItems()
{
    // Iterate through all items and rename them...
    int index = m_spinBox->value();
    foreach (const KFileItem& item, m_items) {
        QString newName = indexedName(m_newName, index, QLatin1Char('#'));
        ++index;

        const KUrl oldUrl = item.url();
        const QString extension = KMimeType::extractKnownExtension(oldUrl.prettyUrl().toLower());
        if (!extension.isEmpty()) {
            newName.append(QLatin1Char('.'));
            newName.append(extension);
        }

        if (oldUrl.fileName() != newName) {
            KUrl newUrl = oldUrl;
            newUrl.setFileName(KIO::encodeFileName(newName));

            QWidget* widget = parentWidget();
            if (!widget) {
                widget = this;
            }

            KonqOperations::rename(widget, oldUrl, newUrl);
        }
    }
}

QString RenameDialog::indexedName(const QString& name, int index, const QChar& indexPlaceHolder)
{
    QString newName = name;

    QString indexString = QString::number(index);

    // Insert leading zeros if necessary
    const int minIndexLength = name.count(indexPlaceHolder);
    while (indexString.length() < minIndexLength) {
        indexString.prepend(QLatin1Char('0'));
    }

    // Replace the index placeholders by the indexString
    const int placeHolderStart = newName.indexOf(indexPlaceHolder);
    newName.replace(placeHolderStart, minIndexLength, indexString);

    return newName;
}

#include "renamedialog.moc"
