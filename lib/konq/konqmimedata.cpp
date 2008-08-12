/* This file is part of the KDE projects
   Copyright (C) 2005 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqmimedata.h"
#include <QtCore/QMimeData>
#include <kdebug.h>

void KonqMimeData::populateMimeData( QMimeData* mimeData,
                                     const KUrl::List& _kdeURLs,
                                     const KUrl::List& mostLocalURLs,
                                     bool cut )
{
    mostLocalURLs.populateMimeData( mimeData );

    KUrl::List kdeURLs( _kdeURLs );
    if ( !kdeURLs.isEmpty() )
    {
        if ( cut ) {
            kdeURLs = simplifiedUrlList( kdeURLs );
        }
        QMimeData tmpMimeData;
        kdeURLs.populateMimeData(&tmpMimeData);
        mimeData->setData("application/x-kde-urilist",tmpMimeData.data("text/uri-list"));
    }

    QByteArray cutSelectionData = cut ? "1" : "0";
    mimeData->setData( "application/x-kde-cutselection", cutSelectionData );

    // for compatibility reasons
    QString application_x_qiconlist;
    int items=qMax(kdeURLs.count(),mostLocalURLs.count());
    for (int i=0;i<items;i++) {
	int offset=i*16;
	QString tmp("%1$@@$%2$@@$32$@@$32$@@$%3$@@$%4$@@$32$@@$16$@@$no data$@@$");
	tmp=tmp.arg(offset).arg(offset).arg(offset).arg(offset+40);
	application_x_qiconlist+=tmp;
    }
    mimeData->setData("application/x-qiconlist",application_x_qiconlist.toLatin1());
    kDebug(1203)<<"setting application/x-qiconlist to "<<application_x_qiconlist;

}

bool KonqMimeData::decodeIsCutSelection( const QMimeData *mimeData )
{
    QByteArray a = mimeData->data( "application/x-kde-cutselection" );
    if ( a.isEmpty() )
        return false;
    else
    {
        kDebug(1203) << "KonqDrag::decodeIsCutSelection : a=" << a;
        return (a.at(0) == '1'); // true if 1
    }
}

KUrl::List KonqMimeData::simplifiedUrlList( const KUrl::List &urls )
{
    KUrl::List ret( urls );
    KUrl::List::const_iterator it = urls.begin();
    while ( it != urls.end() ) {
        KUrl::List::const_iterator it2 = urls.begin();
        while ( it2 != urls.end() ) {
            if ( it != it2 && it->isParentOf( *it2 ) ) {
                ret.removeAll( *it2 );
            }
            ++it2;
        }
        ++it;
    }
    return ret;
}
