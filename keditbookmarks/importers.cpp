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

#include <qregexp.h>
#include <kdebug.h>
#include <klocale.h>

#include <kmessagebox.h>
#include <kfiledialog.h>

#include <kbookmarkmanager.h>

#include <kbookmarkimporter.h>
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>

#include "commands.h"
#include "toplevel.h"
#include "listview.h"
#include "mymanager.h"

#include "importers.h"

QString ImportCommand::name() const {
   return i18n("Import %1 Bookmarks").arg(visibleName());
}

QString ImportCommand::folder() const {
   return m_folder ? i18n("%1 Bookmarks").arg(visibleName()) : QString::null;
}

ImportCommand* ImportCommand::importerFactory(const QCString &type) {
   if (type == "Galeon") return new GaleonImportCommand();
   else if (type == "IE") return new IEImportCommand();
   else if (type == "KDE2") return new KDE2ImportCommand();
   else if (type == "Opera") return new OperaImportCommand();
   else if (type == "Moz") return new MozImportCommand();
   else if (type == "NS") return new NSImportCommand();
   else {
      kdError() << "ImportCommand::importerFactory() - invalid type (" << type << ")!" << endl;
      return 0;
   }
}

ImportCommand* ImportCommand::performImport(const QCString &type, QWidget *top) {
   ImportCommand *importer = ImportCommand::importerFactory(type);

   QString mydirname = importer->requestFilename();
   if (mydirname.isEmpty()) {
      delete importer;
      return 0;
   }

   int answer = 
      KMessageBox::questionYesNoCancel(
         top, i18n("Import as a new subfolder or replace all the current bookmarks?"),
         i18n("%1 Import").arg(importer->visibleName()), 
         i18n("As New Folder"), i18n("Replace"));

   if (answer == KMessageBox::Cancel) {
      delete importer;
      return 0;
   }

   importer->import(mydirname, answer == KMessageBox::Yes);
   return importer;
}

void ImportCommand::execute() {
   KBookmarkGroup bkGroup;

   if (!folder().isNull()) {
      doCreateHoldingFolder(bkGroup);

   } else {
      // import into the root, after cleaning it up
      bkGroup = CurrentMgr::self()->mgr()->root();
      delete m_cleanUpCmd;
      m_cleanUpCmd = DeleteCommand::deleteAll(bkGroup);

      // unselect current item, it doesn't exist anymore
      ListView::self()->clearSelection();
      m_cleanUpCmd->execute();

      // import at the root
      m_group = "";
   }

   doExecute(bkGroup);
}

void ImportCommand::unexecute() {
   if ( !folder().isEmpty() ) {
      // we created a group -> just delete it
      DeleteCommand cmd(m_group);
      cmd.execute();

   } else {
      // we imported at the root -> delete everything
      KBookmarkGroup root = CurrentMgr::self()->mgr()->root();
      KCommand *cmd = DeleteCommand::deleteAll(root);

      // unselect current item, it doesn't exist anymore
      ListView::self()->clearSelection();
      cmd->execute();
      delete cmd;

      // and recreate what was there before
      m_cleanUpCmd->unexecute();
   }
}

//////////////////////////////

void KBookmarkDomBuilder::connectImporter(const QObject *importer) {
   connect(importer, SIGNAL( newBookmark(const QString &, const QCString &, const QString &) ),
                     SLOT( newBookmark(const QString &, const QCString &, const QString &) ));
   connect(importer, SIGNAL( newFolder(const QString &, bool, const QString &) ),
                     SLOT( newFolder(const QString &, bool, const QString &) ));
   connect(importer, SIGNAL( newSeparator() ),
                     SLOT( newSeparator() ) );
   connect(importer, SIGNAL( endFolder() ),
                     SLOT( endFolder() ) );
}

KBookmarkDomBuilder::KBookmarkDomBuilder(const KBookmarkGroup &bkGroup) {
   m_stack.push(&bkGroup);
}

KBookmarkDomBuilder::~KBookmarkDomBuilder() {
   m_list.clear();
   m_stack.clear();
}

void KBookmarkDomBuilder::newBookmark(const QString &text, const QCString &url, const QString &additionnalInfo) {
   KBookmark bk = m_stack.top()->addBookmark(
                                    CurrentMgr::self()->mgr(),
                                    text, QString::fromUtf8(url),
                                    QString::null, false);
   // store additionnal info
   bk.internalElement().setAttribute("netscapeinfo", additionnalInfo);
}

void KBookmarkDomBuilder::newFolder( const QString & text, bool open, const QString & additionnalInfo ) {
   // we use a qvaluelist so that we keep pointers to valid objects in the stack
   m_list.append(m_stack.top()->createNewFolder(CurrentMgr::self()->mgr(), text, false));
   m_stack.push(&(m_list.last()));

   // store additionnal info
   QDomElement element = m_list.last().internalElement();
   element.setAttribute("netscapeinfo", additionnalInfo);
   element.setAttribute("folded", (open?"no":"yes"));
}

void KBookmarkDomBuilder::newSeparator() {
   m_stack.top()->createNewSeparator();
}

void KBookmarkDomBuilder::endFolder() {
   m_stack.pop();
}


//////////////////////////////


void ImportCommand::doCreateHoldingFolder(KBookmarkGroup &bkGroup) {
   bkGroup = CurrentMgr::self()->mgr()
      ->root().createNewFolder(CurrentMgr::self()->mgr(), folder(), false);
   bkGroup.internalElement().setAttribute("icon", m_icon);
   m_group = bkGroup.address();
}

/* -------------------------------------- */

QString OperaImportCommand::requestFilename() const {
   return KOperaBookmarkImporter::operaBookmarksFile();
}

void OperaImportCommand::doExecute(const KBookmarkGroup &bkGroup) {
   KBookmarkDomBuilder builder(bkGroup);
   KOperaBookmarkImporter importer(m_fileName);
   builder.connectImporter(&importer);
   importer.parseOperaBookmarks();
}

/* -------------------------------------- */

QString IEImportCommand::requestFilename() const {
   return KIEBookmarkImporter::IEBookmarksDir();
}

void IEImportCommand::doExecute(const KBookmarkGroup &bkGroup) {
   KBookmarkDomBuilder builder(bkGroup);
   KIEBookmarkImporter importer(m_fileName);
   builder.connectImporter(&importer);
   importer.parseIEBookmarks();
}

/* -------------------------------------- */

void HTMLImportCommand::doExecute(const KBookmarkGroup &bkGroup) {
   KBookmarkDomBuilder builder(bkGroup);
   KNSBookmarkImporter importer(m_fileName);
   builder.connectImporter(&importer);
   importer.parseNSBookmarks(m_utf8);
}

/* -------------------------------------- */

QString MozImportCommand::requestFilename() const {
   return KNSBookmarkImporter::mozillaBookmarksFile();
}

/* -------------------------------------- */

QString NSImportCommand::requestFilename() const {
   return KNSBookmarkImporter::netscapeBookmarksFile();
}

/* -------------------------------------- */

void XBELImportCommand::doCreateHoldingFolder(KBookmarkGroup &) {
   // rather than reuse the old group node we transform the 
   // root xbel node into the group when doing an xbel import
}

void XBELImportCommand::doExecute(const KBookmarkGroup &/*bkGroup*/) {
   KBookmarkManager *pManager;

   // check if already open first???
   pManager = KBookmarkManager::managerForFile(m_fileName, false);

   QDomDocument doc = CurrentMgr::self()->mgr()->internalDocument();

   kdDebug() << 4 << endl;

   // get the xbel
   QDomNode subDoc = pManager->internalDocument().namedItem("xbel").cloneNode();

   if (!folder().isEmpty()) {
      kdDebug() << 3 << endl;
      // transform into folder
      subDoc.toElement().setTagName("folder");

      // clear attributes
      QStringList tags;
      for (uint i = 0; i < subDoc.attributes().count(); i++) {
         tags << subDoc.attributes().item(i).toAttr().name();
      }

      for (QStringList::Iterator it = tags.begin(); it != tags.end(); ++it) {
         subDoc.attributes().removeNamedItem((*it));
      }

      subDoc.toElement().setAttribute("icon", m_icon);

      // give the folder a name
      QDomElement textElem = doc.createElement("title");
      subDoc.insertBefore(textElem, subDoc.firstChild());
      textElem.appendChild(doc.createTextNode(folder()));
   }

   // import and add it
   QDomNode node = doc.importNode(subDoc, true);

   if (!folder().isEmpty()) {
      kdDebug() << 2 << endl;
      CurrentMgr::self()->mgr()->root().internalElement().appendChild(node);
      m_group = KBookmarkGroup(node.toElement()).address();

   } else {
      kdDebug() << 1 << endl;

      QDomElement root = CurrentMgr::self()->mgr()->root().internalElement();

      QValueList<QDomElement> childList;

      QDomNode n = subDoc.firstChild().toElement();

      while (!n.isNull()) {
         QDomElement e = n.toElement();
         if (!e.isNull()) {
            kdDebug() << e.tagName() << endl;
            childList.append(e);
         }
         n = n.nextSibling();
      }

      QValueList<QDomElement>::Iterator it = childList.begin();
      QValueList<QDomElement>::Iterator end = childList.end();
      for (; it!= end ; ++it) {
         root.appendChild((*it));
      }
   }
}

/* -------------------------------------- */

QString GaleonImportCommand::requestFilename() const {
   return KFileDialog::getOpenFileName(
               QDir::homeDirPath() + "/.galeon",
               i18n("*.xbel|Galeon bookmark files (*.xbel)"));
}

/* -------------------------------------- */

QString KDE2ImportCommand::requestFilename() const {
   // locateLocal on the bookmarks file and get dir?
   return KFileDialog::getOpenFileName(
               QDir::homeDirPath() + "/.kde",
               i18n("*.xml|KDE bookmark files (*.xml)"));
}

#include "importers.moc"
