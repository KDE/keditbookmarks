/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@gmail.com>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "dolphinitemcategorizer.h"

#include "dolphinview.h"

#ifdef HAVE_NEPOMUK
#include <config-nepomuk.h>
#include <nepomuk/global.h>
#include <nepomuk/resource.h>
#endif

#include <kdatetime.h>
#include <kdirmodel.h>
#include <kfileitem.h>
#include <klocale.h>
#include <kurl.h>

#include <QtGui/QSortFilterProxyModel>

DolphinItemCategorizer::DolphinItemCategorizer() :
    KItemCategorizer()
{
}

DolphinItemCategorizer::~DolphinItemCategorizer()
{
}

QString DolphinItemCategorizer::categoryForItem(const QModelIndex& index,
                                                int sortRole)
{
    QString retString;

    if (!index.isValid())
    {
        return retString;
    }

    const KDirModel *dirModel = qobject_cast<const KDirModel*>(index.model());
    KFileItem *item = dirModel->itemForIndex(index);

    switch (sortRole)
    {
        case DolphinView::SortByName:
        {
            // KDirModel checks columns to know to which role are
            // we talking about
            QModelIndex theIndex = index.model()->index(index.row(),
                                                        KDirModel::Name,
                                                        index.parent());

            if (!theIndex.isValid()) {
                return retString;
            }

            QVariant data = theIndex.model()->data(theIndex, Qt::DisplayRole);
            if (data.toString().size())
            {
                if (!item->isHidden() && data.toString().at(0).isLetter())
                    retString = data.toString().toUpper().at(0);
                else if (item->isHidden() && data.toString().at(0) == '.' &&
                         data.toString().at(1).isLetter())
                    retString = data.toString().toUpper().at(1);
                else if (item->isHidden() && data.toString().at(0) == '.' &&
                         !data.toString().at(1).isLetter())
                    retString = i18n("Others");
                else if (item->isHidden() && data.toString().at(0) != '.')
                    retString = data.toString().toUpper().at(0);
                else if (item->isHidden())
                    retString = data.toString().toUpper().at(0);
                else
                {
                    bool validCategory = false;

                    const QString str(data.toString().toUpper());
                    const QChar* currA = str.unicode();
                    while (!currA->isNull() && !validCategory) {
                        if (currA->isLetter())
                            validCategory = true;
                        else if (currA->isDigit())
                            return i18n("Others");
                        else
                            ++currA;
                    }

                    if (!validCategory)
                        retString = i18n("Others");
                    else
                        retString = *currA;
                }
            }
            break;
        }

        case DolphinView::SortByDate:
        {
            KDateTime modifiedTime;
            modifiedTime.setTime_t(item->time(KIO::UDS_MODIFICATION_TIME));
            modifiedTime = modifiedTime.toLocalZone();

            if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) == 0)
                retString = i18n("Today");
            else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) == 1)
                retString = i18n("Yesterday");
            else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) < 7)
                retString = i18n("Less than a week");
            else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) < 31)
                retString = i18n("Less than a month");
            else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) < 365)
                retString = i18n("Less than a year");
            else
                retString = i18n("More than a year");
            break;
        }

        case DolphinView::SortByPermissions:
            retString = item->permissionsString();
            break;

        case DolphinView::SortByOwner:
            retString = item->user();
            break;

        case DolphinView::SortByGroup:
            retString = item->group();
            break;

        case DolphinView::SortBySize: {
            const int fileSize = item ? item->size() : -1;
            if (item && item->isDir()) {
                retString = i18n("Folders");
            } else if (fileSize < 5242880) {
                retString = i18nc("Size", "Small");
            } else if (fileSize < 10485760) {
                retString = i18nc("Size", "Medium");
            } else {
                retString = i18nc("Size", "Big");
            }
            break;
        }

        case DolphinView::SortByType:
            retString = item->mimeComment();
            break;

#ifdef HAVE_NEPOMUK
        case DolphinView::SortByRating: {
            KFileItem* item = dirModel->itemForIndex(index);
            if (item != 0) {
                const Nepomuk::Resource resource(item->url().url(), Nepomuk::NFO::File());
                const quint32 rating = resource.rating();
                retString = i18np("1 star", "%1 stars", rating);
            }
            break;
        }
        case DolphinView::SortByTags:
            break;
#endif
    }

    return retString;
}
