/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

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

#include <stdlib.h>

#include <qclipboard.h>
#include <qpopupmenu.h>
#include <qpainter.h>

#include <klocale.h>
#include <dcopclient.h>
#include <kdebug.h>
#include <kapplication.h>

#include <kaction.h>
#include <kstdaction.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kkeydialog.h>
#include <kmessagebox.h>
#include <krun.h>

#include <kicondialog.h>
#include <kiconloader.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kbookmarkexporter.h>
#include <kbookmarkimporter.h>
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>

#include <klineeditdlg.h>

#include "toplevel.h"
#include "commands.h"
#include "importers.h"
#include "core.h"
#include "favicons.h"
#include "testlink.h"
#include "listview.h"
#include "mymanager.h"

#include "dcop.h"

#define top KEBTopLevel::self()

KBookmarkEditorIface::KBookmarkEditorIface()
 : QObject(), DCOPObject("KBookmarkEditor") {

   connectDCOPSignal(0, "KBookmarkNotifier", "addedBookmark(QString,QString,QString,QString,QString)", "slotDcopAddedBookmark(QString,QString,QString,QString,QString)", false);
   connectDCOPSignal(0, "KBookmarkNotifier", "createdNewFolder(QString,QString,QString)", "slotDcopCreatedNewFolder(QString,QString,QString)", false);
}

void KBookmarkEditorIface::slotDcopCreatedNewFolder(QString filename, QString text, QString address) {
   if (top->modified() && filename == MyManager::self()->path()) {
      kdDebug() << "slotDcopCreatedNewFolder(" << text << "," << address << ")" << endl;
      CreateCommand *cmd = new CreateCommand( 
                                  MyManager::self()->correctAddress(address), 
                                  text, QString::null, 
                                  true /*open*/, true /*indirect*/);
      top->addCommand(cmd);
   }
}

void KBookmarkEditorIface::slotDcopAddedBookmark(QString filename, QString url, QString text, QString address, QString icon) {
   if (top->modified() && filename == MyManager::self()->path()) {
      kdDebug() << "slotDcopAddedBookmark(" << url << "," << text << "," << address << "," << icon << ")" << endl;
      CreateCommand *cmd = new CreateCommand(
                                  MyManager::self()->correctAddress(address), 
                                  text, icon, KURL(url), true /*indirect*/);
      top->addCommand(cmd);
   }
}

#include "dcop.moc"
