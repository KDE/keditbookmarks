// -*- tab-width:4; indent-tabs-mode:t -*-
/**
 * kbookmarkmerger.cpp - Copyright (C) 2005 Frerich Raabe <raabe@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <kaboutdata.h>
#include <kapplication.h>
#include <kbookmarkmanager.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>

#include <QtCore/QDir>
#include <QtXml/qdom.h>

int main( int argc, char**argv )
{
	KAboutData aboutData( "kbookmarkmerger", "keditbookmarks", ki18n( "KBookmarkMerger" ),
	                      "1.0", ki18n( "Merges bookmarks installed by 3rd parties into the user's bookmarks" ),
	                      KAboutData::License_BSD,
	                      ki18n(  "Copyright © 2005 Frerich Raabe" ) );
	aboutData.addAuthor( ki18n("Frerich Raabe"), ki18n( "Original author" ),
	                     "raabe@kde.org" );

	KCmdLineArgs::init( argc, argv, &aboutData );

	KCmdLineOptions cmdLineOptions;
	cmdLineOptions.add("+directory", ki18n( "Directory to scan for extra bookmarks" ));
	KCmdLineArgs::addCmdLineOptions( cmdLineOptions );

	KApplication app( false );
	app.disableSessionManagement();

	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
	if ( args->count() != 1 ) {
		kError() << "No directory to scan for bookmarks specified." << endl;
		return 1;
	}

	KBookmarkManager *konqBookmarks = KBookmarkManager::userBookmarksManager();
	QStringList mergedFiles;
	{
		KBookmarkGroup root = konqBookmarks->root();
		for ( KBookmark bm = root.first(); !bm.isNull(); bm = root.next( bm ) ) {
			if ( bm.isGroup() ) {
				continue;
			}

			QString mergedFrom = bm.metaDataItem( "merged_from" );
			if ( !mergedFrom.isNull() ) {
				mergedFiles << mergedFrom;
			}
		}
	}

	bool didMergeBookmark = false;

	QString extraBookmarksDirName = args->arg( 0 );
	QDir extraBookmarksDir( extraBookmarksDirName, "*.xml" );
	if ( !extraBookmarksDir.isReadable() ) {
		kError() << "Failed to read files in directory " << extraBookmarksDirName << endl;
		return 1;
	}

	for ( unsigned int i = 0; i < extraBookmarksDir.count(); ++i ) {
		const QString fileName = extraBookmarksDir[ i ];
		if ( mergedFiles.contains( fileName ) ) {
			continue;
		}

		const QString absPath = extraBookmarksDir.filePath( fileName );
		KBookmarkManager *mgr = KBookmarkManager::managerForFile( absPath, QString() );
		KBookmarkGroup root = mgr->root();
		for ( KBookmark bm = root.first(); !bm.isNull(); bm = root.next( bm ) ) {
			if ( bm.isGroup() ) {
				continue;
			}
			bm.setMetaDataItem( "merged_from", fileName );
			konqBookmarks->root().addBookmark( bm );
			didMergeBookmark = true;
		}
	}

	if ( didMergeBookmark ) {
		konqBookmarks->emitChanged( konqBookmarks->root() ); // calls save
		// see TODO in emitChanged... if it returns false, it would be nice to return 1
		// here.
	}
	return 0;
}

