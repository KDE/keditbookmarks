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

#include "dolphinmodel.h"

#include "dolphinsortfilterproxymodel.h"

#include "kcategorizedview.h"

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
#include <nepomuk/global.h>
#include <nepomuk/resource.h>
#include <nepomuk/tag.h>
#endif

#include <kdatetime.h>
#include <kdirmodel.h>
#include <kfileitem.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>
#include <kuser.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <kpixmapeffect.h>

#include <QList>
#include <QSortFilterProxyModel>
#include <QPainter>
#include <QDir>

DolphinModel::DolphinModel(QObject *parent)
    : KDirModel(parent)
{
}

DolphinModel::~DolphinModel()
{
}

QVariant DolphinModel::data(const QModelIndex &index, int role) const
{
    if (role == KCategorizedSortFilterProxyModel::CategoryRole)
    {
        QString retString;

        if (!index.isValid())
        {
            return retString;
        }

        const KDirModel *dirModel = qobject_cast<const KDirModel*>(index.model());
        KFileItem item = dirModel->itemForIndex(index);

        switch (index.column())
        {
            case KDirModel::Name:
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
                    if (!item.isHidden() && data.toString().at(0).isLetter())
                        retString = data.toString().toUpper().at(0);
                    else if (item.isHidden() && data.toString().at(0) == '.' &&
                            data.toString().at(1).isLetter())
                        retString = data.toString().toUpper().at(1);
                    else if (item.isHidden() && data.toString().at(0) == '.' &&
                            !data.toString().at(1).isLetter())
                        retString = i18nc("@title:group Name", "Others");
                    else if (item.isHidden() && data.toString().at(0) != '.')
                        retString = data.toString().toUpper().at(0);
                    else if (item.isHidden())
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
                                return i18nc("@title:group", "Others");
                            else
                                ++currA;
                        }

                        if (!validCategory)
                            retString = i18nc("@title:group Name", "Others");
                        else
                            retString = *currA;
                    }
                }
                break;
            }

            case KDirModel::Size: {
                const int fileSize = !item.isNull() ? item.size() : -1;
                if (!item.isNull() && item.isDir()) {
                    retString = i18nc("@title:group Size", "Folders");
                } else if (fileSize < 5242880) {
                    retString = i18nc("@title:group Size", "Small");
                } else if (fileSize < 10485760) {
                    retString = i18nc("@title:group Size", "Medium");
                } else {
                    retString = i18nc("@title:group Size", "Big");
                }
                break;
            }

            case KDirModel::ModifiedTime:
            {
                KDateTime modifiedTime;
                modifiedTime.setTime_t(item.time(KIO::UDSEntry::UDS_MODIFICATION_TIME));
                modifiedTime = modifiedTime.toLocalZone();

                if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) == 0)
                    retString = i18nc("@title:group Date", "Today");
                else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) == 1)
                    retString = i18nc("@title:group Date", "Yesterday");
                else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) < 7)
                    retString = i18nc("@title:group Date", "Less than a week");
                else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) < 31)
                    retString = i18nc("@title:group Date", "Less than a month");
                else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) < 365)
                    retString = i18nc("@title:group Date", "Less than a year");
                else
                    retString = i18nc("@title:group Date", "More than a year");
                break;
            }

            case KDirModel::Permissions:
                retString = item.permissionsString();
                break;

            case KDirModel::Owner:
                retString = item.user();
                break;

            case KDirModel::Group:
                retString = item.group();
                break;

            case KDirModel::Type:
                retString = item.mimeComment();
                break;

#ifdef HAVE_NEPOMUK
            case DolphinModel::Rating: {
                const quint32 rating = ratingForIndex(index);

                retString = QString::number(rating);
                break;
            }

            case DolphinModel::Tags: {
                retString = tagsForIndex(index);

                if (retString.isEmpty())
                    retString = i18nc("@title:group Tags", "Not yet tagged");

                break;
            }
#endif
        }

        return retString;
    }

    return KDirModel::data(index, role);
}

int DolphinModel::columnCount(const QModelIndex &parent) const
{
    return KDirModel::columnCount(parent) + (ExtraColumnCount - ColumnCount);
}

quint32 DolphinModel::ratingForIndex(const QModelIndex& index)
{
#ifdef HAVE_NEPOMUK
    quint32 rating = 0;

    const DolphinModel* dolphinModel = static_cast<const DolphinModel*>(index.model());
    KFileItem item = dolphinModel->itemForIndex(index);
    if (!item.isNull()) {
        const Nepomuk::Resource resource(item.url().url(), Nepomuk::NFO::File());
        rating = resource.rating();
    }
    return rating;
#else
    Q_UNUSED(index);
    return 0;
#endif
}

QString DolphinModel::tagsForIndex(const QModelIndex& index)
{
#ifdef HAVE_NEPOMUK
    QString tagsString;

    const DolphinModel* dolphinModel = static_cast<const DolphinModel*>(index.model());
    KFileItem item = dolphinModel->itemForIndex(index);
    if (!item.isNull()) {
        const Nepomuk::Resource resource(item.url().url(), Nepomuk::NFO::File());
        const QList<Nepomuk::Tag> tags = resource.tags();
        QStringList stringList;
        foreach (const Nepomuk::Tag& tag, tags) {
            stringList.append(tag.label());
        }
        stringList.sort();

        foreach (const QString& str, stringList) {
            tagsString += str;
            tagsString += ", ";
        }

        if (!tagsString.isEmpty()) {
            tagsString.resize(tagsString.size() - 2);
        }
    }

    return tagsString;
#else
    Q_UNUSED(index);
    return QString();
#endif
}
