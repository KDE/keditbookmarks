/*  This file is part of the KDE libraries
    Copyright (C) 2002 Simon MacMullen

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

// $Id$

#include <qdict.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qbitmap.h>
#include <qimage.h>

#include <kfileivi.h>
#include <kfileitem.h>
#include <kapplication.h>
#include <kdirlister.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <konq_settings.h>
#include <klocale.h>
#include <kdebug.h>

#include "kivdirectoryoverlay.h"

KIVDirectoryOverlay::KIVDirectoryOverlay(KFileIVI* directory)
: m_lister(0), m_foundItems(false),
  m_containsFolder(false), m_popularIcons(0)
{
    if (!m_lister)
    {
        m_lister = new KDirLister;
        connect(m_lister, SIGNAL(completed()), SLOT(slotCompleted()));
        connect(m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(slotNewItems( const KFileItemList& )));
        m_lister->setShowingDotFiles(false);
    }
    m_directory = directory;
}

KIVDirectoryOverlay::~KIVDirectoryOverlay()
{
    if (m_lister) m_lister->stop();
    delete m_lister;
    delete m_popularIcons;
}

void KIVDirectoryOverlay::start()
{
    if ( m_directory->item()->isReadable() ) {
        m_popularIcons = new QDict<int>;
        m_popularIcons->setAutoDelete(true);
        m_lister->openURL(m_directory->item()->url());
    } else {
        emit finished();
    }
}

void KIVDirectoryOverlay::timerEvent(QTimerEvent *)
{
    m_lister->stop();
}

void KIVDirectoryOverlay::slotCompleted()
{
    if (!m_popularIcons) return;

    // Look through the histogram for the most popular mimetype
    QDictIterator<int> currentIcon( (*m_popularIcons) );
    int best = 0;

    for ( ; currentIcon.current(); ++currentIcon ) {
        int currentCount = (*currentIcon.current());
        if ( best < currentCount ) {
            best = currentCount;
            m_bestIcon = currentIcon.currentKey();
        }
    }

    // Only show folder if there's no other candidate. Most folders contain
    // folders. We know this.
    if ( m_bestIcon.isNull() && m_containsFolder ) {
        m_bestIcon = "folder";
    }

    if ( best * 2 < m_popularIcons -> count() ) {
        m_bestIcon = "kmultiple";
    }

    if (!m_bestIcon.isNull()) {
        m_directory->setOverlay(m_bestIcon);
    }

    delete m_popularIcons;
    m_popularIcons = 0;

    emit finished();
}

void KIVDirectoryOverlay::slotNewItems( const KFileItemList& items )
{
    if ( !m_popularIcons) return;

    KFileItemListIterator files( items );

    KFileItem* file;
    for ( ; (file = files.current()) != 0; ++files ) {
        if ( file -> isFile() ) {

        QString iconName = file -> iconName();
        if (!iconName) continue;

        int* iconCount = m_popularIcons -> find( file -> iconName() );
        if (!iconCount) {
            iconCount = new int(0);
            Q_ASSERT(file);
            m_popularIcons -> insert(file -> iconName(), iconCount);
        }
        (*iconCount)++;
        } else if ( file -> isDir() ) {
            m_containsFolder = true;
        }
    }

    m_foundItems = true;
}

#include "kivdirectoryoverlay.moc"
