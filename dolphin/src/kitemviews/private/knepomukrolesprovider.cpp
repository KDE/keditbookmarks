/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "knepomukrolesprovider.h"

#include <KDebug>
#include <KGlobal>
#include <KLocale>

#include <Nepomuk2/Resource>
#include <Nepomuk2/Tag>
#include <Nepomuk2/Types/Property>
#include <Nepomuk2/Variant>

#include <QTime>

struct KNepomukRolesProviderSingleton
{
    KNepomukRolesProvider instance;
};
K_GLOBAL_STATIC(KNepomukRolesProviderSingleton, s_nepomukRolesProvider)


KNepomukRolesProvider& KNepomukRolesProvider::instance()
{
    return s_nepomukRolesProvider->instance;
}

KNepomukRolesProvider::~KNepomukRolesProvider()
{
}

QSet<QByteArray> KNepomukRolesProvider::roles() const
{
    return m_roles;
}

QHash<QByteArray, QVariant> KNepomukRolesProvider::roleValues(const Nepomuk2::Resource& resource,
                                                              const QSet<QByteArray>& roles) const
{
    if (!resource.isValid()) {
        return QHash<QByteArray, QVariant>();
    }

    QHash<QByteArray, QVariant> values;

    int width = -1;
    int height = -1;

    QHashIterator<QUrl, Nepomuk2::Variant> it(resource.properties());
    while (it.hasNext()) {
        it.next();

        const Nepomuk2::Types::Property property = it.key();
        const QByteArray role = m_roleForUri.value(property.uri());
        if (role.isEmpty() || !roles.contains(role)) {
            continue;
        }

        const Nepomuk2::Variant value = it.value();

        if (role == "imageSize") {
            // Merge the two Nepomuk properties for width and height
            // as one string into the "imageSize" role
            const QString uri = property.uri().toString();
            if (uri.endsWith(QLatin1String("#width"))) {
                width = value.toInt();
            } else if (uri.endsWith(QLatin1String("#height"))) {
                height = value.toInt();
            }

            if (width >= 0 && height >= 0) {
                const QString widthAndHeight = QString::number(width) +
                                               QLatin1String(" x ") +
                                               QString::number(height);
                values.insert(role, widthAndHeight);
            }
        } else if (role == "tags") {
            const QString tags = tagsFromValues(value.toStringList());
            values.insert(role, tags);
        } else if (role == "orientation") {
            const QString orientation = orientationFromValue(value.toInt());
            values.insert(role, orientation);
        } else if (role == "duration") {
            const QString duration = durationFromValue(value.toInt());
            values.insert(role, duration);
        } else if (value.isResource()) {
            const Nepomuk2::Resource resource = value.toResource();
            values.insert(role, resource.genericLabel());
        } else {
            values.insert(role, value.toString());
        }
    }

    return values;
}

KNepomukRolesProvider::KNepomukRolesProvider() :
    m_roles(),
    m_roleForUri()
{
    struct UriInfo
    {
        const char* const uri;
        const char* const role;
    };

    // Mapping from the URIs to the KFileItemModel roles. Note that this must not be
    // a 1:1 mapping: One role may contain several URI-values (e.g. the URIs for height and
    // width of an image are mapped to the role "imageSize")
    static const UriInfo uriInfoList[] = {
        { "http://www.semanticdesktop.org/ontologies/2007/08/15/nao#numericRating", "rating" },
        { "http://www.semanticdesktop.org/ontologies/2007/08/15/nao#hasTag",        "tags" },
        { "http://www.semanticdesktop.org/ontologies/2007/08/15/nao#description",   "comment" },
        { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#wordCount",     "wordCount" },
        { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#lineCount",     "lineCount" },
        { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#width",         "imageSize" },
        { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#height",        "imageSize" },
        { "http://www.semanticdesktop.org/ontologies/2007/05/10/nexif#orientation", "orientation", },
        { "http://www.semanticdesktop.org/ontologies/2009/02/19/nmm#performer",     "artist" },
        { "http://www.semanticdesktop.org/ontologies/2009/02/19/nmm#musicAlbum",    "album" },
        { "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#duration",      "duration" },
        { "http://www.semanticdesktop.org/ontologies/2009/02/19/nmm#trackNumber",   "track" },
        { "http://www.semanticdesktop.org/ontologies/2010/04/30/ndo#copiedFrom",    "copiedFrom" }
    };

    for (unsigned int i = 0; i < sizeof(uriInfoList) / sizeof(UriInfo); ++i) {
        m_roleForUri.insert(QUrl(uriInfoList[i].uri), uriInfoList[i].role);
        m_roles.insert(uriInfoList[i].role);
    }
}

QString KNepomukRolesProvider::tagsFromValues(const QStringList& values) const
{
    QString tags;

    for (int i = 0; i < values.count(); ++i) {
        if (i > 0) {
            tags.append(QLatin1String(", "));
        }

        const Nepomuk2::Tag tag(values[i]);
        tags += tag.genericLabel();
    }

    return tags;
}

QString KNepomukRolesProvider::orientationFromValue(int value) const
{
    QString string;
    switch (value) {
    case 1: string = i18nc("@item:intable Image orientation", "Unchanged"); break;
    case 2: string = i18nc("@item:intable Image orientation", "Horizontally flipped"); break;
    case 3: string = i18nc("@item:intable image orientation", "180° rotated"); break;
    case 4: string = i18nc("@item:intable image orientation", "Vertically flipped"); break;
    case 5: string = i18nc("@item:intable image orientation", "Transposed"); break;
    case 6: string = i18nc("@item:intable image orientation", "90° rotated"); break;
    case 7: string = i18nc("@item:intable image orientation", "Transversed"); break;
    case 8: string = i18nc("@item:intable image orientation", "270° rotated"); break;
    default:
        break;
    }
    return string;
}

QString KNepomukRolesProvider::durationFromValue(int value) const
{
    QTime duration;
    duration = duration.addMSecs(value);
    return duration.toString("hh:mm:ss");
}

