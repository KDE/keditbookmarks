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
*/

#ifndef __KONQ_INFOLISTVIEWWIDGET_H__
#define __KONQ_INFOLISTVIEWWIDGET_H__

#include "konq_listviewwidget.h"

#include <kurl.h>
#include <qmap.h>
#include <qpair.h>

namespace KIO {class MetaInfoJob;};
class KonqListView;
class KSelectAction;

/**
 * The info list view
 */
class KonqInfoListViewWidget : public KonqBaseListViewWidget
{
//   friend class KonqTextViewItem;
   Q_OBJECT
   public:
      KonqInfoListViewWidget( KonqListView *parent, QWidget *parentWidget );
      ~KonqInfoListViewWidget();
      
     const QStringList columnKeys() {return m_columnKeys;}
      
      virtual bool openURL( const KURL &url );

   protected slots:
      // slots connected to the directory lister
//      virtual void setComplete();
      virtual void slotNewItems( const KFileItemList & );
      virtual void slotSelectMimeType();
      
      void slotMetaInfo(const KFileItem*);
      void slotMetaInfoResult();
      
   protected:
       void determineCounts(const KFileItemList& list);
       void rebuildView();
   
      virtual void createColumns();
      void createFavoriteColumns();
      
      /**
       * @internal
       */
      struct KonqILVMimeType
      {
          KonqILVMimeType() : mimetype(0), count(0), hasPlugin(false) {};

          KMimeType::Ptr  mimetype;
          int             count;
          bool            hasPlugin;
      };

      // all the mimetypes
      QMap<QString, KonqILVMimeType > m_counts; 
      QStringList                     m_columnKeys;

      KonqILVMimeType                 m_favorite;
      
      KSelectAction*                  m_mtSelector;
      KIO::MetaInfoJob*               m_metaInfoJob;
};

#endif
