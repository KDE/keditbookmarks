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

#include "konq_textviewitem.h"
#include "konq_propsview.h"
#include "konq_listview.h"

#include <kio/global.h>
#include <assert.h>
#include <stdio.h>

QString KonqTextViewItem::key( int _column, bool asc) const
{
   if (_column==0) return key(1,asc);
   QString tmp=sortChar;
   if (!asc && (sortChar=='0')) tmp=QChar('2');
   //check if it is a time column
   if (_column>1)
   {
      for (unsigned int i=0; i<KonqBaseListViewWidget::NumberOfAtoms; i++)
      {
         ColumnInfo *cInfo=&m_pTextView->columnConfigInfo()[i];
         if (_column==cInfo->displayInColumn)
         {
            if ((cInfo->udsId==KIO::UDS_MODIFICATION_TIME)
                || (cInfo->udsId==KIO::UDS_ACCESS_TIME)
                || (cInfo->udsId==KIO::UDS_CREATION_TIME))
            {
               QString tmpDate;
               tmpDate.sprintf("%ld",m_fileitem->time(cInfo->udsId));
               tmp+=tmpDate;
               return tmp;
            }
            else break;

         };
      };
   };
   tmp+=text(_column);
   return tmp;
};

void KonqTextViewItem::updateContents()
{
   QString tmp;
   long int size=m_fileitem->size();
   mode_t m=m_fileitem->mode();
   if (m_fileitem->isLink())
   {
      if (S_ISDIR(m))
      {
         sortChar='0';
         type=KTVI_DIRLINK;
         tmp="~";
      }
      else if (S_ISREG(m))
      {
         tmp="@";
         type=KTVI_REGULARLINK;
      }
      else
      {
         tmp="!";
         type=KTVI_UNKNOWN;
         size=-1;
      };
   }
   else if (S_ISREG(m))
   {
      if ((m_fileitem->permissions() & (S_IXUSR|S_IXGRP|S_IXOTH)) !=0 )
      {
         tmp="*";
         type=KTVI_EXEC;
      }
      else
      {
         tmp="";
         type=KTVI_REGULAR;
      };
   }
   else if (S_ISDIR(m))
   {
      type=KTVI_DIR;
      tmp="/";
      sortChar='0';
   }
   else if (S_ISCHR(m))
   {
      type=KTVI_CHARDEV;
      tmp="-";
   }
   else if (S_ISBLK(m))
   {
      type=KTVI_BLOCKDEV;
      tmp="+";
   }
   else if (S_ISSOCK(m))
   {
      type=KTVI_SOCKET;
      tmp="=";
   }
   else if (S_ISFIFO(m))
   {
      type=KTVI_FIFO;
      tmp=">";
   }
   else
   {
      tmp="!";
      type=KTVI_UNKNOWN;
      size=-1;
   };
   setText(0,tmp);
   setText(1,m_fileitem->text());
   //now we have the first two columns, so let's do the rest

   for (unsigned int i=0; i<KonqBaseListViewWidget::NumberOfAtoms; i++)
   {
      ColumnInfo *tmpColumn=&m_pTextView->confColumns[i];
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
         case KIO::UDS_LINK_DEST:
            setText(tmpColumn->displayInColumn,m_fileitem->linkDest());
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
         case KIO::UDS_SIZE:
            tmp.sprintf("%9ld",size);
            setText(tmpColumn->displayInColumn,tmp);
            break;
         case KIO::UDS_ACCESS:
            setText(tmpColumn->displayInColumn,makeAccessString(m_fileitem->permissions()));
            break;
         case KIO::UDS_MODIFICATION_TIME:
         case KIO::UDS_ACCESS_TIME:
         case KIO::UDS_CREATION_TIME:
            for( KIO::UDSEntry::ConstIterator it = m_fileitem->entry().begin(); it != m_fileitem->entry().end(); it++ )
            {
               if ((*it).m_uds==(unsigned int)tmpColumn->udsId)
               {
                  QDateTime dt;
                  dt.setTime_t((time_t) (*it).m_long);
                  setText(tmpColumn->displayInColumn,KGlobal::locale()->formatDateTime(dt));
                  break;
               };

            };
            break;
         default:
            break;
         };
      };
   };
};

void KonqTextViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
   if (!m_pTextView->props()->bgPixmap().isNull())
      _painter->drawTiledPixmap( 0, 0, _width, height(),m_pTextView->props()->bgPixmap(),0, 0 );
   QColorGroup cg( _cg );
   cg.setColor(QColorGroup::Text, m_pTextView->colors[type]);
   cg.setColor(QColorGroup::HighlightedText, m_pTextView->highlight[type]);
   cg.setColor(QColorGroup::Highlight, Qt::darkGray);

   //         cg.setColor( QColorGroup::Base, Qt::color0 );
   QListViewItem::paintCell( _painter, cg, _column, _width, _alignment );
};

void KonqTextViewItem::setup()
{
   widthChanged();
   int h(listView()->fontMetrics().height());
   if ( h % 2 > 0 ) h++;
   setHeight(h);
};
