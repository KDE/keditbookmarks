// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "globalbookmarkmanager.h"
#include "importers.h"
#include "kbookmarkmodel/commandhistory.h"
#include "keditbookmarks_version.h"
#include "toplevel.h"

#include "keditbookmarks_debug.h"
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include <KAboutData>
#include <Kdelibs4ConfigMigrator>

#include <KMessageBox>
#include <KWindowSystem>

#include <KBookmarkManager>
#include <QStandardPaths>
#include <kbookmarkexporter.h>
#include <toplevel_interface.h>

// TODO - make this register() or something like that and move dialog into main
static bool askUser(const QString &filename, bool &readonly)
{
    QString interfaceName = QStringLiteral("org.kde.keditbookmarks");
    QString appId = interfaceName + '-' + QString().setNum(QApplication::applicationPid());

    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusReply<QStringList> reply = dbus.interface()->registeredServiceNames();
    if (!reply.isValid())
        return true;
    const QStringList allServices = reply;
    for (QStringList::const_iterator it = allServices.begin(), end = allServices.end(); it != end; ++it) {
        const QString service = *it;
        if (service.startsWith(interfaceName) && service != appId) {
            org::kde::keditbookmarks keditbookmarks(service, QStringLiteral("/keditbookmarks"), dbus);
            QDBusReply<QString> bookmarks = keditbookmarks.bookmarkFilename();
            QString name;
            if (bookmarks.isValid())
                name = bookmarks;
            if (name == filename) {
                int ret = KMessageBox::warningYesNo(nullptr,
                                                    i18n("Another instance of %1 is already running. Do you really "
                                                         "want to open another instance or continue work in the same instance?\n"
                                                         "Please note that, unfortunately, duplicate views are read-only.",
                                                         QGuiApplication::applicationDisplayName()),
                                                    i18nc("@title:window", "Warning"),
                                                    KGuiItem(i18n("Run Another")), /* yes */
                                                    KGuiItem(i18n("Continue in Same")) /*  no */);
                if (ret == KMessageBox::No) {
                    QDBusInterface keditinterface(service, QStringLiteral("/keditbookmarks/MainWindow_1"));
                    // TODO fix me
                    QDBusReply<qlonglong> value = keditinterface.call(QDBus::NoBlock, QStringLiteral("winId"));
                    qlonglong id = 0;
                    if (value.isValid())
                        id = value;
                    ////qCDebug(KEDITBOOKMARKS_LOG)<<" id !!!!!!!!!!!!!!!!!!! :"<<id;
                    KWindowSystem::activateWindow((WId)id);
                    return false;
                } else if (ret == KMessageBox::Yes) {
                    readonly = true;
                }
            }
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    Kdelibs4ConfigMigrator migrate(QStringLiteral("keditbookmarks"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("keditbookmarksrc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("keditbookmarksuirc"));
    migrate.migrate();

    KLocalizedString::setApplicationDomain("keditbookmarks");

    KAboutData aboutData(QStringLiteral("keditbookmarks"),
                         i18n("Bookmark Editor"),
                         QStringLiteral(KEDITBOOKMARKS_VERSION_STRING),
                         i18n("Bookmark Organizer and Editor"),
                         KAboutLicense::GPL,
                         i18n("Copyright 2000-2017, KDE developers"));
    aboutData.addAuthor(i18n("David Faure"), i18n("Initial author"), QStringLiteral("faure@kde.org"));
    aboutData.addAuthor(i18n("Alexander Kellett"), i18n("Author"), QStringLiteral("lypanov@kde.org"));

    aboutData.setOrganizationDomain(QByteArray("kde.org"));
    aboutData.setDesktopFileName(QStringLiteral("org.kde.keditbookmarks"));
    KAboutData::setApplicationData(aboutData);

    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("bookmarks-organize")));

    QCommandLineParser parser;
    parser.setApplicationDescription(aboutData.shortDescription());
    // clang-format off
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("importmoz"), i18n("Import bookmarks from a file in Mozilla format"), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("importns"), i18n("Import bookmarks from a file in Netscape (4.x and earlier) format"), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("importie"), i18n("Import bookmarks from a file in Internet Explorer's Favorites format"), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("importopera"), i18n("Import bookmarks from a file in Opera format"), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("importkde3"), i18n("Import bookmarks from a file in KDE2 format"), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("importgaleon"), i18n("Import bookmarks from a file in Galeon format"), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("exportmoz"), i18n("Export bookmarks to a file in Mozilla format"), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("exportns"), i18n("Export bookmarks to a file in Netscape (4.x and earlier) format"), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("exporthtml"), i18n("Export bookmarks to a file in a printable HTML format"), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("exportie"), i18n("Export bookmarks to a file in Internet Explorer's Favorites format"), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("exportopera"), i18n("Export bookmarks to a file in Opera format"), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("address"), i18n("Open at the given position in the bookmarks file"), QStringLiteral("address")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("customcaption"), i18n("Set the user-readable caption, for example \"Konsole\""), QStringLiteral("caption")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("nobrowser"), i18n("Hide all browser related functions")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("dbusObjectName"), i18n("A unique name that represents this bookmark collection, usually the kinstance name.\n"
                                 "This should be \"konqueror\" for the Konqueror bookmarks, \"kfile\" for KFileDialog bookmarks, etc.\n"
                                 "The final D-Bus object path is /KBookmarkManager/<dbusObjectName>"), QStringLiteral("name")));
    // clang-format on
    parser.addPositionalArgument(QStringLiteral("[file]"), i18n("File to edit"));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    const bool isGui = !(parser.isSet(QStringLiteral("exportmoz")) || parser.isSet(QStringLiteral("exportns")) || parser.isSet(QStringLiteral("exporthtml")) //
                         || parser.isSet(QStringLiteral("exportie")) || parser.isSet(QStringLiteral("exportopera")) //
                         || parser.isSet(QStringLiteral("importmoz")) || parser.isSet(QStringLiteral("importns")) //
                         || parser.isSet(QStringLiteral("importie")) || parser.isSet(QStringLiteral("importopera")) //
                         || parser.isSet(QStringLiteral("importkde3")) || parser.isSet(QStringLiteral("importgaleon")));

    const bool browser = !parser.isSet(QStringLiteral("nobrowser"));

    // enable high dpi support
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    const bool gotFilenameArg = (parser.positionalArguments().count() == 1);

    QString filename = gotFilenameArg ? parser.positionalArguments().at(0)
                                      : QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/konqueror/bookmarks.xml");

    if (!isGui) {
        GlobalBookmarkManager::self()->createManager(filename, QString(), new CommandHistory());
        GlobalBookmarkManager::ExportType exportType = GlobalBookmarkManager::MozillaExport; // uumm.. can i just set it to -1 ?
        int got = 0;
        const char *arg, *arg2 = nullptr, *importType = nullptr;
        if (arg = "exportmoz", parser.isSet(arg)) {
            exportType = GlobalBookmarkManager::MozillaExport;
            arg2 = arg;
            got++;
        }
        if (arg = "exportns", parser.isSet(arg)) {
            exportType = GlobalBookmarkManager::NetscapeExport;
            arg2 = arg;
            got++;
        }
        if (arg = "exporthtml", parser.isSet(arg)) {
            exportType = GlobalBookmarkManager::HTMLExport;
            arg2 = arg;
            got++;
        }
        if (arg = "exportie", parser.isSet(arg)) {
            exportType = GlobalBookmarkManager::IEExport;
            arg2 = arg;
            got++;
        }
        if (arg = "exportopera", parser.isSet(arg)) {
            exportType = GlobalBookmarkManager::OperaExport;
            arg2 = arg;
            got++;
        }
        if (arg = "importmoz", parser.isSet(arg)) {
            importType = "Moz";
            arg2 = arg;
            got++;
        }
        if (arg = "importns", parser.isSet(arg)) {
            importType = "NS";
            arg2 = arg;
            got++;
        }
        if (arg = "importie", parser.isSet(arg)) {
            importType = "IE";
            arg2 = arg;
            got++;
        }
        if (arg = "importopera", parser.isSet(arg)) {
            importType = "Opera";
            arg2 = arg;
            got++;
        }
        if (arg = "importgaleon", parser.isSet(arg)) {
            importType = "Galeon";
            arg2 = arg;
            got++;
        }
        if (arg = "importkde3", parser.isSet(arg)) {
            importType = "KDE2";
            arg2 = arg;
            got++;
        }
        if (!importType && arg2) {
            Q_ASSERT(arg2);
            // TODO - maybe an xbel export???
            if (got > 1) { // got == 0 isn't possible as !isGui is dependent on "export.*"
                qCWarning(KEDITBOOKMARKS_LOG) << i18n("You may only specify a single --export option.");
                return 1;
            }
            QString path = parser.value(arg2);
            GlobalBookmarkManager::self()->doExport(exportType, path);
        } else if (importType) {
            if (got > 1) { // got == 0 isn't possible as !isGui is dependent on "import.*"
                qCWarning(KEDITBOOKMARKS_LOG) << i18n("You may only specify a single --import option.");
                return 1;
            }
            QString path = parser.value(arg2);
            KBookmarkModel *model = GlobalBookmarkManager::self()->model();
            ImportCommand *importer = ImportCommand::importerFactory(model, importType);
            importer->import(path, true);
            importer->redo();
            GlobalBookmarkManager::self()->managerSave();
            GlobalBookmarkManager::self()->notifyManagers();
        }
        return 0; // error flag on exit?, 1?
    }

    QString address = parser.isSet(QStringLiteral("address")) ? parser.value(QStringLiteral("address")) : QStringLiteral("/0");

    QString caption = parser.isSet(QStringLiteral("customcaption")) ? parser.value(QStringLiteral("customcaption")) : QString();

    QString dbusObjectName;
    if (parser.isSet(QStringLiteral("dbusObjectName"))) {
        dbusObjectName = parser.value(QStringLiteral("dbusObjectName"));
    } else {
        if (gotFilenameArg)
            dbusObjectName = QString();
        else
            dbusObjectName = QStringLiteral("konqueror");
    }

    bool readonly = false; // passed by ref

    if (askUser((gotFilenameArg ? filename : QString()), readonly)) {
        KEBApp *toplevel = new KEBApp(filename, readonly, address, browser, caption, dbusObjectName);
        toplevel->setAttribute(Qt::WA_DeleteOnClose);
        toplevel->show();
        return app.exec();
    }

    return 0;
}
