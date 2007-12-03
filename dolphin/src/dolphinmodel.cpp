/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
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

#include <QList>
#include <QSortFilterProxyModel>
#include <QPainter>
#include <QDir>
#include <QFileInfo>

DolphinModel::DolphinModel(QObject *parent)
    : KDirModel(parent)
{
}

DolphinModel::~DolphinModel()
{
}

QVariant DolphinModel::data(const QModelIndex &index, int role) const
{
    if (role == KCategorizedSortFilterProxyModel::CategoryDisplayRole)
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

                retString = modifiedTime.toString(i18nc("Prints out the month and year: %B is full month name in current locale, and %Y is full year number", "%B, %Y"));
                break;
            }

            case KDirModel::Permissions:
            {
                QString user;
                QString group;
                QString others;

                QFileInfo info(item.url().pathOrUrl());

                if (info.permission(QFile::ReadUser))
                    user = i18n("Read, ");

                if (info.permission(QFile::WriteUser))
                    user += i18n("Write, ");

                if (info.permission(QFile::ExeUser))
                    user += i18n("Execute, ");

                if (user.isEmpty())
                    user = i18n("Forbidden");
                else
                    user = user.mid(0, user.count() - 2);

                if (info.permission(QFile::ReadGroup))
                    group = i18n("Read, ");

                if (info.permission(QFile::WriteGroup))
                    group += i18n("Write, ");

                if (info.permission(QFile::ExeGroup))
                    group += i18n("Execute, ");

                if (group.isEmpty())
                    group = i18n("Forbidden");
                else
                    group = group.mid(0, group.count() - 2);

                if (info.permission(QFile::ReadOther))
                    others = i18n("Read, ");

                if (info.permission(QFile::WriteOther))
                    others += i18n("Write, ");

                if (info.permission(QFile::ExeOther))
                    others += i18n("Execute, ");

                if (others.isEmpty())
                    others = i18n("Forbidden");
                else
                    others = others.mid(0, others.count() - 2);

                retString = i18nc("This shows files and folders permissions: user, group and others", "(User: %1) (Group: %2) (Others: %3)", user, group, others);
                break;
            }

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
    else if (role == KCategorizedSortFilterProxyModel::CategorySortRole)
    {
#if 0
        QVariant retVariant;

        if (!index.isValid())
        {
            return retVariant;
        }

        const KDirModel *dirModel = qobject_cast<const KDirModel*>(index.model());
        KFileItem item = dirModel->itemForIndex(index);

        switch (index.column()) {
        case KDirModel::Name: {
            bool leftFileNameStartsByLetter = false;
            const QString str(leftFileItem.name().toUpper());
            const QChar* currA = str.unicode();
            while (!currA->isNull() && !leftFileNameStartsByLetter) {
                if (currA->isLetter())
                    leftFileNameStartsByLetter = true;
                else if (currA->isDigit()) {
                    break;
                } else
                    ++currA;
            }

            bool rightFileNameStartsByLetter = false;
            const QString strb(rightFileItem.name().toUpper());
            const QChar *currB = strb.unicode();
            while (!currB->isNull() && !rightFileNameStartsByLetter) {
                if (currB->isLetter())
                    rightFileNameStartsByLetter = true;
                else if (currB->isDigit()) {
                    break;
                } else
                    ++currB;
            }

            if (!rightFileNameStartsByLetter)
                return -1;

            if (!leftFileNameStartsByLetter && rightFileNameStartsByLetter)
                return 1;

            return naturalCompare(*currA, *currB);
        }

        case KDirModel::Size: {
            // If we are sorting by size, show folders first. We will sort them
            // correctly later.
            if (leftFileItem.isDir() && !rightFileItem.isDir()) {
                return -1;
            }

            if (!leftFileItem.isDir() && rightFileItem.isDir()) {
                return 1;
            }

            if (leftFileItem.isDir() && rightFileItem.isDir()) {
                return 0;
            }

            const int leftFileSize = !leftFileItem.isNull() ? leftFileItem.size() : -1;
            const int rightFileSize = !rightFileItem.isNull() ? rightFileItem.size() : -1;
            int leftGroup;
            int rightGroup;

            if (leftFileSize < 5242880) {
                leftGroup = 0;
            } else if (leftFileSize < 10485760) {
                leftGroup = 1;
            } else {
                leftGroup = 2;
            }

            if (rightFileSize < 5242880) {
                rightGroup = 0;
            } else if (rightFileSize < 10485760) {
                rightGroup = 1;
            } else {
                rightGroup = 2;
            }

            return leftGroup - rightGroup;
        }

        case KDirModel::ModifiedTime: {
            KDateTime leftTime = leftFileItem.time(KFileItem::ModificationTime);
            KDateTime rightTime = rightFileItem.time(KFileItem::ModificationTime);

            if ((leftTime.date().year() == rightTime.date().year()) &&
                (leftTime.date().month() == rightTime.date().month())) {
                return 0;
            }

            if (leftTime > rightTime) {
                return 1;
            }

            return -1;
        }

        case KDirModel::Permissions: {
            QFileInfo leftFileInfo(leftFileItem.url().pathOrUrl());
            QFileInfo rightFileInfo(rightFileItem.url().pathOrUrl());

            int leftPermissionsPoints = pointsForPermissions(leftFileInfo);
            int rightPermissionsPoints = pointsForPermissions(rightFileInfo);

            return leftPermissionsPoints - rightPermissionsPoints;
        }

        case KDirModel::Owner: {
            return naturalCompare(leftFileItem.user().toLower(),
                                rightFileItem.user().toLower());
        }

        case KDirModel::Group: {
            return naturalCompare(leftFileItem.group().toLower(),
                                rightFileItem.group().toLower());
        }

        case KDirModel::Type: {
            // If we are sorting by size, show folders first. We will sort them
            // correctly later.
            if (leftFileItem.isDir() && !rightFileItem.isDir()) {
                return -1;
            } else if (!leftFileItem.isDir() && rightFileItem.isDir()) {
                return 1;
            }

            return naturalCompare(leftFileItem.mimeComment().toLower(),
                                rightFileItem.mimeComment().toLower());
        }

#ifdef HAVE_NEPOMUK
        case DolphinView::SortByRating: {
            const qint32 leftRating = DolphinModel::ratingForIndex(left);
            const qint32 rightRating = DolphinModel::ratingForIndex(right);
            return leftRating - rightRating;
        }

        case DolphinView::SortByTags: {
            const QString leftTags = DolphinModel::tagsForIndex(left);
            const QString rightTags = DolphinModel::tagsForIndex(right);

            if (leftTags.isEmpty() && !rightTags.isEmpty())
                return 1;
            else if (!leftTags.isEmpty() && rightTags.isEmpty())
                return -1;

            return naturalCompare(DolphinModel::tagsForIndex(left), DolphinModel::tagsForIndex(right)) < 0;
        }
#endif

        default:
            break;

        }
#endif
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

                retString = modifiedTime.toString(i18nc("Prints out the month and year: %B is full month name in current locale, and %Y is full year number", "%B, %Y"));
                break;
            }

            case KDirModel::Permissions:
            {
                QString user;
                QString group;
                QString others;

                QFileInfo info(item.url().pathOrUrl());

                if (info.permission(QFile::ReadUser))
                    user = i18n("Read, ");

                if (info.permission(QFile::WriteUser))
                    user += i18n("Write, ");

                if (info.permission(QFile::ExeUser))
                    user += i18n("Execute, ");

                if (user.isEmpty())
                    user = i18n("Forbidden");
                else
                    user = user.mid(0, user.count() - 2);

                if (info.permission(QFile::ReadGroup))
                    group = i18n("Read, ");

                if (info.permission(QFile::WriteGroup))
                    group += i18n("Write, ");

                if (info.permission(QFile::ExeGroup))
                    group += i18n("Execute, ");

                if (group.isEmpty())
                    group = i18n("Forbidden");
                else
                    group = group.mid(0, group.count() - 2);

                if (info.permission(QFile::ReadOther))
                    others = i18n("Read, ");

                if (info.permission(QFile::WriteOther))
                    others += i18n("Write, ");

                if (info.permission(QFile::ExeOther))
                    others += i18n("Execute, ");

                if (others.isEmpty())
                    others = i18n("Forbidden");
                else
                    others = others.mid(0, others.count() - 2);

                retString = i18nc("This shows files and folders permissions: user, group and others", "(User: %1) (Group: %2) (Others: %3)", user, group, others);
                break;
            }

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
