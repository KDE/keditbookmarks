/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kfileitemlistwidget.h"
#include "kfileitemmodel.h"
#include "kitemlistview.h"

#include <kmimetype.h>
#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KIO/MetaData>
#include <QDateTime>

KFileItemListWidgetInformant::KFileItemListWidgetInformant() :
    KStandardItemListWidgetInformant()
{
}

KFileItemListWidgetInformant::~KFileItemListWidgetInformant()
{
}

QString KFileItemListWidgetInformant::itemText(int index, const KItemListView* view) const
{
    Q_ASSERT(qobject_cast<KFileItemModel*>(view->model()));
    KFileItemModel* fileItemModel = static_cast<KFileItemModel*>(view->model());

    const KFileItem item = fileItemModel->fileItem(index);
    return item.text();
}

QString KFileItemListWidgetInformant::roleText(const QByteArray& role,
                                               const QHash<QByteArray, QVariant>& values) const
{
    QString text;
    const QVariant roleValue = values.value(role);

    // Implementation note: In case if more roles require a custom handling
    // use a hash + switch for a linear runtime.

    if (role == "size") {
        if (values.value("isDir").toBool()) {
            // The item represents a directory. Show the number of sub directories
            // instead of the file size of the directory.
            if (!roleValue.isNull()) {
                const int count = roleValue.toInt();
                if (count < 0) {
                    text = i18nc("@item:intable", "Unknown");
                } else {
                    text = i18ncp("@item:intable", "%1 item", "%1 items", count);
                }
            }
        } else {
            const KIO::filesize_t size = roleValue.value<KIO::filesize_t>();
            text = KGlobal::locale()->formatByteSize(size);
        }
    } else if (role == "date") {
        const QDateTime dateTime = roleValue.toDateTime();
        text = KGlobal::locale()->formatDateTime(dateTime);
    } else {
        text = KStandardItemListWidgetInformant::roleText(role, values);
    }

    return text;
}

KFileItemListWidget::KFileItemListWidget(KItemListWidgetInformant* informant, QGraphicsItem* parent) :
    KStandardItemListWidget(informant, parent)
{
}

KFileItemListWidget::~KFileItemListWidget()
{
}

KItemListWidgetInformant* KFileItemListWidget::createInformant()
{
    return new KFileItemListWidgetInformant();
}

bool KFileItemListWidget::isRoleRightAligned(const QByteArray& role) const
{
    return role == "size";
}

bool KFileItemListWidget::isHidden() const
{
    return data().value("text").toString().startsWith(QLatin1Char('.'));
}

QFont KFileItemListWidget::customizedFont(const QFont& baseFont) const
{
    // The customized font should be italic if the file is a symbolic link.
    QFont font(baseFont);
    font.setItalic(data().value("isLink").toBool());
    return font;
}

int KFileItemListWidget::selectionLength(const QString& text) const
{
    // Select the text without MIME-type extension
    int selectionLength = text.length();

    // If item is a directory, use the whole text length for
    // selection (ignore all points)
    if(data().value("isDir").toBool()) {
        return selectionLength;
    }

    const QString extension = KMimeType::extractKnownExtension(text);
    if (extension.isEmpty()) {
        // For an unknown extension just exclude the extension after
        // the last point. This does not work for multiple extensions like
        // *.tar.gz but usually this is anyhow a known extension.
        selectionLength = text.lastIndexOf(QLatin1Char('.'));

        // If no point could be found, use whole text length for selection.
        if (selectionLength < 1) {
            selectionLength = text.length();
        }

    } else {
        selectionLength -= extension.length() + 1;
    }

    return selectionLength;
}

#include "kfileitemlistwidget.moc"
