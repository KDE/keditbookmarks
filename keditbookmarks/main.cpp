// -*- mode:cperl; cperl-indent-level:4; cperl-continued-statement-offset:4; indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
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

#include "toplevel.h"
#include "importers.h"

#include <dcopclient.h>
#include <dcopref.h>
#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kuniqueapplication.h>

#include <kmessagebox.h>
#include <kwin.h>

#include <kbookmarkmanager.h>
#include <kbookmarkexporter.h>

static KCmdLineOptions options[] = {
    {"importmoz <filename>", I18N_NOOP("Import bookmarks from a file in Mozilla format"), 0},
    {"importns <filename>", I18N_NOOP("Import bookmarks from a file in Netscape (4.x and earlier) format"), 0},
    {"importie <filename>", I18N_NOOP("Import bookmarks from a file in Internet Explorer's Favorites format"), 0},
    {"importopera <filename>", I18N_NOOP("Import bookmarks from a file in Opera format"), 0},

    {"exportmoz <filename>", I18N_NOOP("Export bookmarks to a file in Mozilla format"), 0},
    {"exportns <filename>", I18N_NOOP("Export bookmarks to a file in Netscape (4.x and earlier) format"), 0},
    {"exporthtml <filename>", I18N_NOOP("Export bookmarks to a file in a printable HTML format"), 0},
    {"exportie <filename>", I18N_NOOP("Export bookmarks to a file in Internet Explorer's Favorites format"), 0},
    {"exportopera <filename>", I18N_NOOP("Export bookmarks to a file in Opera format"), 0},

    {"address <address>", I18N_NOOP("Open at the given position in the bookmarks file"), 0},
    {"customcaption <caption>", I18N_NOOP("Set the user readable caption for example \"Konsole\""), 0},
    {"nobrowser", I18N_NOOP("Hide all browser related functions"), 0},
    {"+[file]", I18N_NOOP("File to edit"), 0},
    KCmdLineLastOption
};

static void continueInWindow(QString _wname) {
    QCString wname = _wname.latin1();
    int id = -1;

    QCStringList apps = kapp->dcopClient()->registeredApplications();
    for (QCStringList::Iterator it = apps.begin(); it != apps.end(); ++it) {
        QCString &clientId = *it;

        if (qstrncmp(clientId, wname, wname.length()) != 0)
            continue;

        DCOPRef client(clientId.data(), wname + "-mainwindow#1");
        DCOPReply result = client.call("getWinID()");

        if (result.isValid()) {
            id = (int)result;
            break;
        }
    }

    KWin::activateWindow(id);
}

// TODO - make this register() or something like that and move dialog into main
static int askUser(KApplication &app, QString filename, bool &readonly) {
    QCString requestedName("keditbookmarks");

    if (!filename.isEmpty())
        requestedName += "-" + filename.utf8();

    if (app.dcopClient()->registerAs(requestedName, false) == requestedName)
        return true;

    int ret = KMessageBox::warningYesNo(0, 
            i18n("Another instance of %1 is already running, do you really "
                "want to open another instance or continue work in the same instance?\n"
                "Please note that, unfortunately, duplicate views are read-only.").arg(kapp->caption()), 
            i18n("Warning"),
            i18n("Run Another"),     /* yes */
            i18n("Continue in Same") /*  no */);

    if (ret == KMessageBox::No) {
        continueInWindow(requestedName);
        return false;
    } else if (ret == KMessageBox::Yes) {
        readonly = true;
    }

    return true;
}

#include <kactioncollection.h>

extern "C" KDE_EXPORT int kdemain(int argc, char **argv) {
    KLocale::setMainCatalogue("konqueror");
    KAboutData aboutData("keditbookmarks", I18N_NOOP("Bookmark Editor"), "1.2",
            I18N_NOOP("Konqueror Bookmarks Editor"),
            KAboutData::License_GPL,
            I18N_NOOP("(c) 2000 - 2003, KDE developers") );
    aboutData.addAuthor("David Faure", I18N_NOOP("Initial author"), "faure@kde.org");
    aboutData.addAuthor("Alexander Kellett", I18N_NOOP("Maintainer"), "lypanov@kde.org");

    KCmdLineArgs::init(argc, argv, &aboutData);
    KApplication::addCmdLineOptions();
    KCmdLineArgs::addCmdLineOptions(options);

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    bool isGui = !(args->isSet("exportmoz") || args->isSet("exportns") || args->isSet("exporthtml") 
                || args->isSet("exportie") || args->isSet("exportopera")
                || args->isSet("importmoz") || args->isSet("importns")
                || args->isSet("importie") || args->isSet("importopera"));

    bool browser = args->isSet("browser");

    KApplication::disableAutoDcopRegistration(); 
    KApplication app(isGui, isGui);

    bool gotArg = (args->count() == 1);

    QString filename = gotArg
        ? QString::fromLatin1(args->arg(0))
        : locateLocal("data", QString::fromLatin1("konqueror/bookmarks.xml"));

    if (!isGui) {
        CurrentMgr::self()->createManager(filename);
        CurrentMgr::ExportType exportType = CurrentMgr::MozillaExport; // uumm.. can i just set it to -1 ?
        int got = 0;
        const char *arg, *arg2 = 0, *importType = 0;
        if (arg = "exportmoz",   args->isSet(arg)) { exportType = CurrentMgr::MozillaExport;  arg2 = arg; got++; }
        if (arg = "exportns",    args->isSet(arg)) { exportType = CurrentMgr::NetscapeExport; arg2 = arg; got++; }
        if (arg = "exporthtml",  args->isSet(arg)) { exportType = CurrentMgr::HTMLExport;     arg2 = arg; got++; }
        if (arg = "exportie",    args->isSet(arg)) { exportType = CurrentMgr::IEExport;       arg2 = arg; got++; }
        if (arg = "exportopera", args->isSet(arg)) { exportType = CurrentMgr::OperaExport;    arg2 = arg; got++; }
        if (arg = "importmoz",   args->isSet(arg)) { importType = "Moz";   arg2 = arg; got++; }
        if (arg = "importns",    args->isSet(arg)) { importType = "NS";    arg2 = arg; got++; }
        if (arg = "importie",    args->isSet(arg)) { importType = "IE";    arg2 = arg; got++; }
        if (arg = "importopera", args->isSet(arg)) { importType = "Opera"; arg2 = arg; got++; }
        if (!importType && arg2) {
            Q_ASSERT(arg2);
            // TODO - maybe an xbel export???
            if (got > 1) // got == 0 isn't possible as !isGui is dependant on "export.*"
                KCmdLineArgs::usage(I18N_NOOP("You may only specify a single --export option."));
            QString path = QString::fromLocal8Bit(args->getOption(arg2));
            CurrentMgr::self()->doExport(exportType, path);
        } else if (importType) {
            if (got > 1) // got == 0 isn't possible as !isGui is dependant on "import.*"
                KCmdLineArgs::usage(I18N_NOOP("You may only specify a single --import option."));
            QString path = QString::fromLocal8Bit(args->getOption(arg2));
            ImportCommand *importer = ImportCommand::importerFactory(importType);
            importer->import(path, true);
            importer->execute();
            CurrentMgr::self()->managerSave();
            CurrentMgr::self()->notifyManagers();
        }
        return 0; // error flag on exit?, 1?
    }

    QString address = args->isSet("address")
        ? QString::fromLocal8Bit(args->getOption("address"))
        : QString("/0");

    QString caption = args->isSet("customcaption")
        ? QString::fromLocal8Bit(args->getOption("customcaption"))
        : QString::null;

    args->clear();

    bool readonly = false; // passed by ref

    if (askUser(app, (gotArg ? filename : QString::null), readonly)) {
        KEBApp *toplevel = new KEBApp(filename, readonly, address, browser, caption);
        toplevel->show();
        app.setMainWidget(toplevel);
        return app.exec();
    }

    return 0;
}
