/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_listview.h"
#include <konq_settings.h>
#include <kdebug.h>
#include <klocale.h>
#include <assert.h>
#include <stdio.h>
#include <qpainter.h>
#include <qheader.h>
#include <kiconloader.h>

/**************************************************************
 *
 * KonqListViewItem
 *
 **************************************************************/
KonqListViewItem::KonqListViewItem( KonqBaseListViewWidget *_listViewWidget, KonqListViewItem * _parent, KFileItem* _fileitem )
   : KonqBaseListViewItem(_listViewWidget,_parent,_fileitem ),
     m_pixmaps(listView()->columns())
{
   updateContents();
}

KonqListViewItem::KonqListViewItem( KonqBaseListViewWidget *_listViewWidget, KFileItem* _fileitem )
   : KonqBaseListViewItem(_listViewWidget,_fileitem),
     m_pixmaps(listView()->columns())

{
   updateContents();
}

void KonqListViewItem::updateContents()
{
   // Set the pixmap
   setDisabled( m_bDisabled );

   // Set the text of each column
   setText(0,m_fileitem->text());

   // The order is: .dir (0), dir (1), .file (2), file (3)
   sortChar = S_ISDIR( m_fileitem->mode() ) ? 1 : 3;
   if ( m_fileitem->text()[0] == '.' )
       --sortChar;

   //now we have the first column, so let's do the rest

   int numExtra = 1;
   for (unsigned int i=0; i<m_pListViewWidget->NumberOfAtoms; i++)
   {
      ColumnInfo *tmpColumn=&m_pListViewWidget->columnConfigInfo()[i];
      if (tmpColumn->displayThisOne)
      {
         switch (tmpColumn->udsId)
         {
         case KIO::UDS_USER:
            setText(tmpColumn->displayInColumn,m_fileitem->user());
            break;
         case KIO::UDS_GROUP:
            setText(tmpColumn->displayInColumn,m_fileitem->group());
            break;
         case KIO::UDS_FILE_TYPE:
            setText(tmpColumn->displayInColumn,m_fileitem->mimeComment());
            break;
         case KIO::UDS_MIME_TYPE:
            setText(tmpColumn->displayInColumn,m_fileitem->mimetype());
            break;
         case KIO::UDS_URL:
            setText(tmpColumn->displayInColumn,m_fileitem->url().prettyURL());
            break;
         case KIO::UDS_LINK_DEST:
            setText(tmpColumn->displayInColumn,m_fileitem->linkDest());
            break;
         case KIO::UDS_SIZE:
            if ( m_pListViewWidget->m_pSettings->fileSizeInBytes() )
                setText(tmpColumn->displayInColumn,KGlobal::locale()->formatNumber( m_fileitem->size(),0)+" ");
            else
                setText(tmpColumn->displayInColumn,KIO::convertSize(m_fileitem->size())+" ");
            break;
         case KIO::UDS_ACCESS:
            setText(tmpColumn->displayInColumn,makeAccessString(m_fileitem->permissions()));
            break;
         case KIO::UDS_MODIFICATION_TIME:
         case KIO::UDS_ACCESS_TIME:
         case KIO::UDS_CREATION_TIME:
            {
               QDateTime dt;
               time_t _time = m_fileitem->time( tmpColumn->udsId );
               if ( _time != 0 )
               {
                   dt.setTime_t( _time );
                   setText(tmpColumn->displayInColumn,KGlobal::locale()->formatDate(dt.date(),TRUE)+" "+KGlobal::locale()->formatTime(dt.time())+" ");
                   //setText(tmpColumn->displayInColumn,KGlobal::locale()->formatDateTime(dt));
               }
            }
            break;
         case KIO::UDS_EXTRA:
         {
             // extraData[numExtra]
             /// ######## SLOOOOW
             KIO::UDSEntry::ConstIterator it = m_fileitem->entry().begin();
             int n = 0;
             for( ; it != m_fileitem->entry().end(); it++ ) {
                 if ((*it).m_uds == KIO::UDS_EXTRA)
                 {
                     ++n;
                     if ( n == numExtra )
                     {
                         setText(tmpColumn->displayInColumn, (*it).m_str);
                         break;
                     }
                 }
             }

             ++numExtra;
         }
         default:
            break;
         };
      };
   };
}

void KonqListViewItem::setDisabled( bool disabled )
{
    KonqBaseListViewItem::setDisabled( disabled );
    int iconSize = m_pListViewWidget->iconSize();
    iconSize = iconSize ? iconSize : KGlobal::iconLoader()->currentSize( KIcon::Small ); // Default = small
    setPixmap( 0, m_fileitem->pixmap( iconSize, state() ) );
}

void KonqListViewItem::setActive( bool active )
{
    if ( m_bActive == active )
        return;

    //#### Optimize away repaint if possible, like the iconview does?
    KonqBaseListViewItem::setActive( active );
    int iconSize = m_pListViewWidget->iconSize();
    iconSize = iconSize ? iconSize : KGlobal::iconLoader()->currentSize( KIcon::Small ); // Default = small
    setPixmap( 0, m_fileitem->pixmap( iconSize, state() ) );
}

void KonqListViewItem::setPixmap( int column, const QPixmap& pm )
{
   if ( column < 0 )
      return;

   const QPixmap *current = pixmap( column );

   if ( ( pm.isNull() && !current ) ||
        ( current && pm.serialNumber() == current->serialNumber() ) )
      return;

   int oldWidth = current ? current->width() : 0;
   int oldHeight = current ? current->height() : 0;

   if ( m_pixmaps.size() <= column )
      m_pixmaps.resize( column+1 );

   delete current;
   m_pixmaps[column] = pm.isNull() ? 0 : new QPixmap( pm );

   int newWidth = pm.isNull() ? 0 : pm.width();
   int newHeight = pm.isNull() ? 0 : pm.height();

   // If the height or width have changed then we're going to have to repaint
   // this whole thing.  Fortunately since most of the calls are coming from
   // setActive() this is the uncommon case.

   if ( oldWidth != newWidth || oldHeight != newHeight )
   {
      setup();
      widthChanged( column );
      invalidateHeight();
      return;
   }

   // If we're just replacing the icon with another one its size -- i.e. a
   // "highlighted" icon, don't bother repainting the whole widget.

   QListView *lv = m_pListViewWidget;

   int decorationWidth = lv->treeStepSize() * ( depth() + ( lv->rootIsDecorated() ? 1 : 0 ) );
   int x = lv->header()->sectionPos( column ) + decorationWidth + lv->itemMargin();
   int y = lv->itemPos( this );
   int w = newWidth;
   int h = height();
   lv->repaintContents( x, y, w, h );
}

const QPixmap* KonqListViewItem::pixmap( int column ) const
{
   bool ok;
   if (m_pixmaps.count() <= column)
      return 0;

   QPixmap *pm = m_pixmaps.at( column, &ok );
   if( !ok )
      return 0;
   return pm;
}

int KonqBaseListViewItem::compare( QListViewItem* item, int col, bool ascending ) const
{
   KonqListViewItem* k = static_cast<KonqListViewItem*>( item );
   if ( sortChar != k->sortChar )
      // Dirs are always first, even when sorting in descending order
      return !ascending ? k->sortChar - sortChar : sortChar - k->sortChar;

   for ( unsigned int i=0; i<m_pListViewWidget->NumberOfAtoms; i++ )
   {
      ColumnInfo *cInfo = &m_pListViewWidget->columnConfigInfo()[i];
      if ( col == cInfo->displayInColumn )
      {
         switch ( cInfo->udsId )
         {
            case KIO::UDS_MODIFICATION_TIME:
            case KIO::UDS_ACCESS_TIME:
            case KIO::UDS_CREATION_TIME:
            {
                time_t t1 = m_fileitem->time( cInfo->udsId );
                time_t t2 = k->m_fileitem->time( cInfo->udsId );
                return ( t1 > t2 ) ? 1 : ( t1 < t2 ) ? -1 : 0;
            }
            case KIO::UDS_SIZE:
            {
                KIO::filesize_t s1 = m_fileitem->size();
                KIO::filesize_t s2 = k->m_fileitem->size();
                return ( s1 > s2 ) ? 1 : ( s1 < s2 ) ? -1 : 0;
            }
            default:
                break;
         }
         break;
      }
   }
   if ( m_pListViewWidget->caseInsensitiveSort() )
       return text( col ).lower().localeAwareCompare( k->text( col ).lower() );
   else {
       return m_pListViewWidget->m_pSettings->caseSensitiveCompare( text( col ), k->text( col ) );
   }
}

void KonqListViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
  QColorGroup cg( _cg );

  if ( _column == 0 )
  {
    _painter->setFont( m_pListViewWidget->itemFont() );
  }

  cg.setColor( QColorGroup::Text, m_pListViewWidget->itemColor() );

  KListViewItem::paintCell( _painter, cg, _column, _width, _alignment );
}

const char* KonqBaseListViewItem::makeAccessString( const mode_t mode)
{
   static char buffer[ 12 ];

   char uxbit,gxbit,oxbit;

   if ( (mode & (S_IXUSR|S_ISUID)) == (S_IXUSR|S_ISUID) )
      uxbit = 's';
   else if ( (mode & (S_IXUSR|S_ISUID)) == S_ISUID )
      uxbit = 'S';
   else if ( (mode & (S_IXUSR|S_ISUID)) == S_IXUSR )
      uxbit = 'x';
   else
      uxbit = '-';

   if ( (mode & (S_IXGRP|S_ISGID)) == (S_IXGRP|S_ISGID) )
      gxbit = 's';
   else if ( (mode & (S_IXGRP|S_ISGID)) == S_ISGID )
      gxbit = 'S';
   else if ( (mode & (S_IXGRP|S_ISGID)) == S_IXGRP )
      gxbit = 'x';
   else
      gxbit = '-';

   if ( (mode & (S_IXOTH|S_ISVTX)) == (S_IXOTH|S_ISVTX) )
      oxbit = 't';
   else if ( (mode & (S_IXOTH|S_ISVTX)) == S_ISVTX )
      oxbit = 'T';
   else if ( (mode & (S_IXOTH|S_ISVTX)) == S_IXOTH )
      oxbit = 'x';
   else
      oxbit = '-';

   buffer[0] = ((( mode & S_IRUSR ) == S_IRUSR ) ? 'r' : '-' );
   buffer[1] = ((( mode & S_IWUSR ) == S_IWUSR ) ? 'w' : '-' );
   buffer[2] = uxbit;
   buffer[3] = ((( mode & S_IRGRP ) == S_IRGRP ) ? 'r' : '-' );
   buffer[4] = ((( mode & S_IWGRP ) == S_IWGRP ) ? 'w' : '-' );
   buffer[5] = gxbit;
   buffer[6] = ((( mode & S_IROTH ) == S_IROTH ) ? 'r' : '-' );
   buffer[7] = ((( mode & S_IWOTH ) == S_IWOTH ) ? 'w' : '-' );
   buffer[8] = oxbit;
   buffer[9] = 0;

   return buffer;
}


KonqBaseListViewItem::KonqBaseListViewItem(KonqBaseListViewWidget *_listViewWidget, KFileItem* _fileitem)
:KListViewItem(_listViewWidget)
,sortChar(0)
,m_bDisabled(false)
,m_bActive(false)
,m_fileitem(_fileitem)
,m_pListViewWidget(_listViewWidget)
{}

KonqBaseListViewItem::KonqBaseListViewItem(KonqBaseListViewWidget *_listViewWidget, KonqBaseListViewItem *_parent, KFileItem* _fileitem)
:KListViewItem(_parent)
,sortChar(0)
,m_bDisabled(false)
,m_bActive(false)
,m_fileitem(_fileitem)
,m_pListViewWidget(_listViewWidget)
{}

KonqBaseListViewItem::~KonqBaseListViewItem()
{
   if (m_pListViewWidget->m_activeItem == this)
      m_pListViewWidget->m_activeItem = 0;
   if (m_pListViewWidget->m_dragOverItem == this)
      m_pListViewWidget->m_dragOverItem = 0;

   if (m_pListViewWidget->m_selected)
      m_pListViewWidget->m_selected->removeRef(this);
}

QRect KonqBaseListViewItem::rect() const
{
    QRect r = m_pListViewWidget->itemRect(this);
    return QRect( m_pListViewWidget->viewportToContents( r.topLeft() ), QSize( r.width(), r.height() ) );
}

void KonqBaseListViewItem::mimetypeFound()
{
    // Update icon
    setDisabled( m_bDisabled );
    uint done = 0;
    KonqBaseListViewWidget * lv = m_pListViewWidget;
    for (unsigned int i=0; i<m_pListViewWidget->NumberOfAtoms && done < 2; i++)
    {
        ColumnInfo *tmpColumn=&lv->columnConfigInfo()[i];
        if (lv->columnConfigInfo()[i].udsId==KIO::UDS_FILE_TYPE && tmpColumn->displayThisOne)
        {
            setText(tmpColumn->displayInColumn, m_fileitem->mimeComment());
            done++;
        }
        if (lv->columnConfigInfo()[i].udsId==KIO::UDS_MIME_TYPE && tmpColumn->displayThisOne)
        {
            setText(tmpColumn->displayInColumn, m_fileitem->mimetype());
            done++;
        }
    }
}

