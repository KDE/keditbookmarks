/* This file is part of the KDE project
   Copyright (C) 1999, 2000, 2001, 2002 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kfileivi.h"
#include "konq_iconviewwidget.h"
#include "konq_operations.h"

#include <qpainter.h>

#include <kurldrag.h>
#include <kiconeffect.h>
#include <kfileitem.h>
#include <kdebug.h>

#undef Bool

/**
 * Private data for KFileIVI
 */
struct KFileIVI::Private
{
    QIconSet icons; // Icon states (cached to prevent re-applying icon effects
		    // every time)
    QPixmap  thumb; // Raw unprocessed thumbnail
    int	     state; // Currently displayed state of the icon
    QString m_animatedIcon; // Name of animation
    bool m_animated;        // Animation currently running ?
};

KFileIVI::KFileIVI( KonqIconViewWidget *iconview, KFileItem* fileitem, int size )
    : KIconViewItem( iconview, fileitem->text(),
		     fileitem->pixmap( size, KIcon::DefaultState ) ),
    m_size( size ), m_state( KIcon::DefaultState ),
    m_bDisabled( false ), m_bThumbnail( false ), m_fileitem( fileitem )
{
    setDropEnabled( S_ISDIR( m_fileitem->mode() ) );
    d = new KFileIVI::Private;

    // Cache entry for the icon effects
    d->icons.reset( *pixmap(), QIconSet::Large );
    d->state = KIcon::DefaultState;
    d->m_animated = false;

    // iconName() requires the mimetype to be known
    if ( fileitem->isMimeTypeKnown() )
        setMouseOverAnimation( fileitem->iconName() );
}

KFileIVI::~KFileIVI()
{
    delete d;
}

void KFileIVI::invalidateThumb( int state, bool redraw )
{
    QIconSet::Mode mode;
    switch( state )
    {
	case KIcon::DisabledState:
	    mode = QIconSet::Disabled;
	    break;
	case KIcon::ActiveState:
	    mode = QIconSet::Active;
	    break;
	case KIcon::DefaultState:
	default:
	    mode = QIconSet::Normal;
	    break;
    }
    d->icons = QIconSet();
    d->icons.setPixmap( KGlobal::iconLoader()->iconEffect()->
			apply( d->thumb, KIcon::Desktop, state ),
			QIconSet::Large, mode );
    d->state = state;

    QIconViewItem::setPixmap( d->icons.pixmap( QIconSet::Large, mode ),
			      false, redraw );
}

void KFileIVI::setIcon( int size, int state, bool recalc, bool redraw )
{
    m_size = size;
    m_bThumbnail = false;
    if ( m_bDisabled )
      m_state = KIcon::DisabledState;
    else
      m_state = state;

    d->state = m_state;
    setPixmapDirect( m_fileitem->pixmap( m_size, m_state ), recalc, redraw );
}

void KFileIVI::setPixmapDirect( const QPixmap& pixmap, bool recalc, bool redraw )
{
    QIconSet::Mode mode;
    switch( d->state )
    {
	case KIcon::DisabledState:
	    mode = QIconSet::Disabled;
	    break;
	case KIcon::ActiveState:
	    mode = QIconSet::Active;
	    break;
	case KIcon::DefaultState:
	default:
	    mode = QIconSet::Normal;
	    break;
    }

    // We cannot just reset() the iconset here, because setIcon can be
    // called with any state and not just normal state. So we just
    // create a dummy empty iconset as base object.
    d->icons = QIconSet();
    d->icons.setPixmap( pixmap, QIconSet::Large, mode );
    QIconViewItem::setPixmap( d->icons.pixmap( QIconSet::Large, mode ),
			      recalc, redraw );
}

void KFileIVI::setDisabled( bool disabled )
{
    if ( m_bDisabled != disabled && !isThumbnail() )
    {
        m_bDisabled = disabled;
        m_state = m_bDisabled ? KIcon::DisabledState : KIcon::DefaultState;
        QIconViewItem::setPixmap( m_fileitem->pixmap( m_size, m_state ), false, true );
    }
}

void KFileIVI::setThumbnailPixmap( const QPixmap & pixmap )
{
    m_bThumbnail = true;
    d->thumb = pixmap;
    // QIconSet::reset() doesn't seem to clear the other generated pixmaps,
    // so we just create a blank QIconSet here
    d->icons = QIconSet();
    d->icons.setPixmap( KGlobal::iconLoader()->iconEffect()->
		    apply( pixmap, KIcon::Desktop, KIcon::DefaultState ),
		    QIconSet::Large, QIconSet::Normal );

    d->state = KIcon::DefaultState;

    // Recalc when setting this pixmap!
    QIconViewItem::setPixmap( d->icons.pixmap( QIconSet::Large,
			      QIconSet::Normal ), true );
}

void KFileIVI::setEffect( int group, int state )
{
    QIconSet::Mode mode;
    switch( state )
    {
	case KIcon::DisabledState:
	    mode = QIconSet::Disabled;
	    break;
	case KIcon::ActiveState:
	    mode = QIconSet::Active;
	    break;
	case KIcon::DefaultState:
	default:
	    mode = QIconSet::Normal;
	    break;
    }
    // Do not update if the fingerprint is identical (prevents flicker)!

    KIconEffect *effect = KGlobal::iconLoader()->iconEffect();

    bool haveEffect = effect->hasEffect( KIcon::Desktop, d->state ) !=
                      effect->hasEffect( KIcon::Desktop, state );

    if( haveEffect &&
        effect->fingerprint( KIcon::Desktop, d->state ) !=
	effect->fingerprint( KIcon::Desktop, state ) )
    {
	// Effects on are not applied until they are first accessed to
	// save memory. Do this now when needed
	if( m_bThumbnail )
	{
	    if( d->icons.isGenerated( QIconSet::Large, mode ) )
		d->icons.setPixmap( effect->apply( d->thumb, KIcon::Desktop, state ),
				    QIconSet::Large, mode );
	}
	else
	{
	    if( d->icons.isGenerated( QIconSet::Large, mode ) )
		d->icons.setPixmap( m_fileitem->pixmap( m_size, state ),
				    QIconSet::Large, mode );
	}
	QIconViewItem::setPixmap( d->icons.pixmap( QIconSet::Large, mode ) );
    }
    d->state = state;
}

void KFileIVI::refreshIcon( bool redraw )
{
    if (!isThumbnail())
        setIcon( m_size, m_state, true, redraw );
}

bool KFileIVI::acceptDrop( const QMimeSource *mime ) const
{
    if ( mime->provides( "text/uri-list" ) ) // We're dragging URLs
    {
        if ( m_fileitem->acceptsDrops() ) // Directory, executables, ...
            return true;
        KURL::List uris;
        if ( iconView()->inherits( "KonqIconViewWidget" ) )
            // Use cache if we can
            uris = ( static_cast<KonqIconViewWidget*>(iconView()) )->dragURLs();
        else
            KURLDrag::decode( mime, uris );

        // Check if we want to drop something on itself
        // (Nothing will happen, but it's a convenient way to move icons)
        KURL::List::Iterator it = uris.begin();
        for ( ; it != uris.end() ; it++ )
        {
            if ( m_fileitem->url().cmp( *it, true /*ignore trailing slashes*/ ) )
                return true;
        }
    }
    return QIconViewItem::acceptDrop( mime );
}

void KFileIVI::setKey( const QString &key )
{
    QString theKey = key;

    QVariant sortDirProp = iconView()->property( "sortDirectoriesFirst" );

    if ( S_ISDIR( m_fileitem->mode() ) && ( !sortDirProp.isValid() || ( sortDirProp.type() == QVariant::Bool && sortDirProp.toBool() ) ) )
      theKey.prepend( iconView()->sortDirection() ? '0' : '1' );
    else
      theKey.prepend( iconView()->sortDirection() ? '1' : '0' );

    QIconViewItem::setKey( theKey );
}

void KFileIVI::dropped( QDropEvent *e, const QValueList<QIconDragItem> & )
{
    KonqOperations::doDrop( item(), item()->url(), e, iconView() );
}

void KFileIVI::returnPressed()
{
    m_fileitem->run();
}

void KFileIVI::paintItem( QPainter *p, const QColorGroup &c )
{
    QColorGroup cg( c );
    cg.setColor( QColorGroup::Text, static_cast<KonqIconViewWidget*>(iconView())->itemColor() );
    if ( m_fileitem->isLink() )
    {
        QFont f( p->font() );
        f.setItalic( TRUE );
        p->setFont( f );
    }

    //*** TEMPORARY CODE - MUST BE MADE CONFIGURABLE FIRST - Martijn
    // SET UNDERLINE ON HOVER ONLY
    /*if ( ( ( KonqIconViewWidget* ) iconView() )->m_pActiveItem == this )
    {
        QFont f( p->font() );
        f.setUnderline( TRUE );
        p->setFont( f );
    }*/

    KIconViewItem::paintItem( p, cg );
}

bool KFileIVI::move( int x, int y )
{
    if ( static_cast<KonqIconViewWidget*>(iconView())->isDesktop() ) {
	if ( x < 5 )
	    x = 5;
	if ( x > iconView()->viewport()->width() - ( width() + 5 ) )
	    x = iconView()->viewport()->width() - ( width() + 5 );
	if ( y < 5 )
	    y = 5;
	if ( y > iconView()->viewport()->height() - ( height() + 5 ) )
	    y = iconView()->viewport()->height() - ( height() + 5 );
    }
    return QIconViewItem::move( x, y );
}

bool KFileIVI::hasAnimation() const
{
    return !d->m_animatedIcon.isEmpty() && !m_bThumbnail;
}

void KFileIVI::setMouseOverAnimation( const QString& movieFileName )
{
    if ( !movieFileName.isEmpty() )
    {
        //kdDebug(1203) << "KIconViewItem::setMouseOverAnimation " << movieFileName << endl;
        d->m_animatedIcon = movieFileName;
    }
}

QString KFileIVI::mouseOverAnimation() const
{
    return d->m_animatedIcon;
}

bool KFileIVI::isAnimated() const
{
    return d->m_animated;
}

void KFileIVI::setAnimated( bool a )
{
    d->m_animated = a;
}

/* vim: set noet sw=4 ts=8 softtabstop=4: */
