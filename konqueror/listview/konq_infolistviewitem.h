/* This file is part of the KDE project
   Copyright (C) 2002 Rolf Magnus <ramagnus@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation version 2.0

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
   
   $Id$
*/

#ifndef __konq_infolistviewitems_h__
#define __konq_infolistviewitems_h__

#include <konq_listview.h>
#include <qstring.h>
#include <kicontheme.h>

// for mode_t
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

class QPainter;
class KFileItem;
class KonqInfoListViewWidget;

/**
 * One item in the info list
 */
class KonqInfoListViewItem : public KonqBaseListViewItem
{
   public:
      /**
       * Create an item in the tree toplevel representing a file
       * @param _parent the parent widget, the tree view
       * @param _fileitem the file item created by KDirLister
       */
      KonqInfoListViewItem( KonqInfoListViewWidget *_listViewWidget, KFileItem* _fileitem );
      /**
       * Create an item representing a file, inside a directory
       * @param _treeview the parent tree view  - now unused
       * @param _parent the parent widget, a directory item in the tree view
       * @param _fileitem the file item created by KDirLister
       */
      KonqInfoListViewItem( KonqInfoListViewWidget *, KonqInfoListViewItem *_parent, KFileItem* _fileitem );

      virtual ~KonqInfoListViewItem() { }

      virtual QString key( int _column, bool ) const;
      virtual void paintCell( QPainter *_painter, const QColorGroup & cg,
                              int column, int width, int alignment );
      virtual void updateContents();
      virtual void setDisabled( bool disabled );
      
      enum ColumnType { FileName, MetaInfo, UDS_Entry };
      
      virtual void gotMetaInfo();
//      virtual QString MetaInfoKey(int column);
//      virtual ColumnType type(int column);
      
   protected:
      /** Parent tree view */
      KonqInfoListViewWidget* m_ILVWidget; // the info list view widget
};

#endif
