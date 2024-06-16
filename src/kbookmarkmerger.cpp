/**
 * SPDX-FileCopyrightText: 2005 Frerich Raabe <raabe@kde.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <KAboutData>

#include <KBookmarkManager>

#include "keditbookmarks_debug.h"

#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QStandardPaths>
#include <qdom.h>

#include "keditbookmarks_version.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("keditbookmarks"));

    KAboutData aboutData(QStringLiteral("kbookmarkmerger"),
                         i18n("KBookmarkMerger"),
                         QStringLiteral(KEDITBOOKMARKS_VERSION_STRING),
                         i18n("Merges bookmarks installed by 3rd parties into the user's bookmarks"),
                         KAboutLicense::BSDL,
                         i18n("Copyright © 2005 Frerich Raabe"));
    aboutData.addAuthor(i18n("Frerich Raabe"), i18n("Original author"), QStringLiteral("raabe@kde.org"));

    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("directory"), i18n("Directory to scan for extra bookmarks"));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (parser.positionalArguments().count() != 1) {
        qCCritical(KEDITBOOKMARKS_LOG) << "No directory to scan for bookmarks specified.";
        return 1;
    }

    const QString bookmarksFile = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/konqueror/bookmarks.xml");
    KBookmarkManager konqBookmarks(bookmarksFile);
    QStringList mergedFiles;
    {
        KBookmarkGroup root = konqBookmarks.root();
        for (KBookmark bm = root.first(); !bm.isNull(); bm = root.next(bm)) {
            if (bm.isGroup()) {
                continue;
            }

            QString mergedFrom = bm.metaDataItem(QStringLiteral("merged_from"));
            if (!mergedFrom.isNull()) {
                mergedFiles << mergedFrom;
            }
        }
    }

    bool didMergeBookmark = false;

    QString extraBookmarksDirName = parser.positionalArguments().at(0);
    QDir extraBookmarksDir(extraBookmarksDirName, QStringLiteral("*.xml"));
    if (!extraBookmarksDir.isReadable()) {
        qCCritical(KEDITBOOKMARKS_LOG) << "Failed to read files in directory " << extraBookmarksDirName;
        return 1;
    }

    for (unsigned int i = 0; i < extraBookmarksDir.count(); ++i) {
        const QString fileName = extraBookmarksDir[i];
        if (mergedFiles.contains(fileName)) {
            continue;
        }

        const QString absPath = extraBookmarksDir.filePath(fileName);
        KBookmarkManager mgr(absPath);
        KBookmarkGroup root = mgr.root();
        for (KBookmark bm = root.first(); !bm.isNull(); bm = root.next(bm)) {
            if (bm.isGroup()) {
                continue;
            }
            bm.setMetaDataItem(QStringLiteral("merged_from"), fileName);
            konqBookmarks.root().addBookmark(bm);
            didMergeBookmark = true;
        }
    }

    if (didMergeBookmark) {
        konqBookmarks.emitChanged(konqBookmarks.root()); // calls save
        // see TODO in emitChanged... if it returns false, it would be nice to return 1
        // here.
    }
    return 0;
}
