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

#ifndef __importers_h
#define __importers_h

#include <klocale.h>
#include <kio/job.h>

#include <kcommand.h>
#include <kbookmark.h>

#include <qptrstack.h>
#include <qobject.h>

// OLD COMMENT

// AK - TODO SOON!!!!

// do something, yet this case is ambigious,
// for a cancel from the file dialog it should
// do nothing, yet for a import without "..."
// it shouldn't even be called + thus should be
// a assert, or possible not... 
// need to think more about this...

// REPLY

// umm.. problem is that in the first place there
// should imo be no imports without "..." as all
// the importers should just suggest rather than
// forcing the filename to import

template <class TheImporter>
TheImporter *callImporter(QWidget *parent)
{
   QString mydirname = TheImporter::requestFilename();
   if (mydirname.isEmpty()) {
      return 0;
   }
   int ret = TheImporter::doImport(parent, TheImporter().visibleName());
   return (ret == 0) ? 0 : new TheImporter(mydirname, (ret == 2));
}

class ImportCommand : public QObject, public KCommand
{
   Q_OBJECT
public:
   /**
    * @param name name of the command
    * @param fileName HTML file to import
    * @param folder name of the folder to create. Empty for no creation (root()).
    * @param icon icon for the new folder, if @p folder isn't empty
    * @param utf8 true if the HTML is in utf-8 encoding
    */
   ImportCommand(const QString &fileName, 
                 bool folder, const QString &icon, bool utf8)
      : KCommand(), m_fileName(fileName), m_folder(folder), 
                    m_icon(icon), m_cleanUpCmd(0L), m_utf8(utf8)
   { ; }

   ImportCommand()
      : KCommand(), m_fileName(QString::null), m_folder(false), 
                    m_icon(QString::null), m_cleanUpCmd(0L), m_utf8(false)
   { ; }

   virtual QString name() const;
   virtual QString visibleName() const = 0;

   static int doImport(QWidget*, QString);

   virtual ~ImportCommand() 
   { ; }

   virtual void execute();
   virtual void unexecute();

   QString groupAddress() { return m_group; }
   QString folder();

protected slots:
   void newBookmark(const QString &text, const QCString &url, const QString &additionnalInfo);
   void newFolder(const QString &text, bool open, const QString &additionnalInfo);
   void newSeparator();
   void endFolder();

protected:
   void connectImporter(const QObject *importer);

   virtual void doCreateHoldingFolder(KBookmarkGroup &bkGroup);
   virtual void doExecuteWrapper(const KBookmarkGroup bkGroup);

   virtual void doExecute() = 0;

   QPtrStack<KBookmarkGroup> m_stack;
   QValueList<KBookmarkGroup> m_list;
   QString m_fileName;
   bool m_folder;
   QString m_icon;
   QString m_group;
   KMacroCommand *m_cleanUpCmd;
   bool m_utf8;
};

class XBELImportCommand : public ImportCommand
{
public:
   XBELImportCommand(const QString &fileName, 
                     bool folder, const QString &icon);
   XBELImportCommand() { ; }
   virtual QString visibleName() const = 0;
private:
   virtual void doCreateHoldingFolder(KBookmarkGroup &bkGroup);
   virtual void doExecuteWrapper(const KBookmarkGroup bkGroup);
   virtual void doExecute();
};

class GaleonImportCommand : public XBELImportCommand
{
public:
   GaleonImportCommand(const QString &fileName, bool folder);
   GaleonImportCommand() { ; }
   virtual QString visibleName() const;
   static QString requestFilename();
private:
};

class KDE2ImportCommand : public XBELImportCommand
{
public:
   KDE2ImportCommand(const QString &fileName, bool folder);
   KDE2ImportCommand() { ; }
   virtual QString visibleName() const;
   static QString requestFilename();
private:
};

class HTMLImportCommand : public ImportCommand
{
public:
   HTMLImportCommand(const QString &fileName, bool folder, const QString &icon, bool utf8);
   HTMLImportCommand() { ; }
   virtual QString visibleName() const = 0;
private:
   virtual void doExecute();
};

class NSImportCommand : public HTMLImportCommand
{
public:
   NSImportCommand(const QString &fileName, bool folder);
   NSImportCommand() { ; }
   virtual QString visibleName() const;
   static QString requestFilename();
private:
};

class MozImportCommand : public HTMLImportCommand
{
public:
   MozImportCommand(const QString &fileName, bool folder);
   MozImportCommand() { ; }
   virtual QString visibleName() const;
   static QString requestFilename();
private:
};

class IEImportCommand : public ImportCommand
{
public:
   IEImportCommand(const QString &fileName, bool folder);
   IEImportCommand() { ; }
   virtual QString visibleName() const;
   static QString requestFilename();
private:
   virtual void doExecute();
};

class OperaImportCommand : public ImportCommand
{
public:
   OperaImportCommand(const QString &fileName, bool folder);
   OperaImportCommand() { ; }
   virtual QString visibleName() const;
   static QString requestFilename();
private:
   virtual void doExecute();
};

#endif
