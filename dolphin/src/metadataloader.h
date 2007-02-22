/***************************************************************************
 *   Copyright (C) 2006 by Oscar Blumberg                                  *
 *   o.blumberg@robertlan.eu.org                                           *
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

#ifndef METADATALOADER_H
#define METADATALOADER_H

namespace Nempomuk {
namespace Backbone {
    class Registry;
}
}
class KUrl;
class QString;

/**
 * @brief Load metadata for specific files.
 * This class uses the KMetaData API to load metadata from the NEPOMUK storage.
 *
 * @author Oscar Blumberg <o.blumberg@robertlan.eu.org>
 */
class MetadataLoader
{
friend class DolphinApplication;
public:
        ~MetadataLoader();
        bool storageUp();

        QString getAnnotation(const KUrl& file);
        void setAnnotation(const KUrl& file, const QString& annotation);

private:
        MetadataLoader();
        bool m_up;
};


#endif
