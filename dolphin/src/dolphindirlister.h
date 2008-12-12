/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#ifndef DOLPHINDIRLISTER_H
#define DOLPHINDIRLISTER_H

#include <kdirlister.h>

/**
 * @brief Extends the class KDirLister by emitting an error
 *        signal containing text.
 *
 * @author Peter Penz
 */
class DolphinDirLister : public KDirLister
{
    Q_OBJECT

public:
    DolphinDirLister();
    virtual ~DolphinDirLister();

signals:
    /** Is emitted whenever an error has occurred. */
    void errorMessage(const QString& msg);

    /** Is emitted when the URL of the directory lister represents a file. */
    void urlIsFileError(const KUrl& url);

protected:
    virtual void handleError(KIO::Job* job);
};

#endif
