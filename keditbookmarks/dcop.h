/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __dcop_h
#define __dcop_h

#include <dcopobject.h>

class KBookmarkEditorIface : public QObject, public DCOPObject
{
   Q_OBJECT
   K_DCOP
public:
   KBookmarkEditorIface();
k_dcop_hidden:
   void slotDcopUpdatedAccessMetadata(QString filename, QString url);
   void slotDcopAddedBookmark(QString filename, QString url, QString text, QString address, QString icon);
   void slotDcopCreatedNewFolder(QString filename, QString text, QString address);
};

#endif
