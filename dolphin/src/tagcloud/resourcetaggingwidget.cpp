/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#include "resourcetaggingwidget.h"
#include "tagcloud.h"
#include "taggingpopup.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QCursor>
#include <QtGui/QAction>

#include <KLocale>


class Nepomuk::ResourceTaggingWidget::Private
{
public:
    Nepomuk::Resource resource;

    TagCloud* resourceTagCloud;
    TaggingPopup* popup;

    QList<Tag> resourceTags;

    QAction* changeTagsAction;

    void showTaggingPopup( const QPoint& );
    void _k_slotShowTaggingPopup();
};


void Nepomuk::ResourceTaggingWidget::Private::showTaggingPopup( const QPoint& pos )
{
    popup->showAllTags();
    resourceTags = resource.tags();
    Q_FOREACH( Tag tag, resourceTags ) {
        popup->setTagSelected( tag, true );
    }

    popup->exec( pos );

    resource.setTags( resourceTags );
}


void Nepomuk::ResourceTaggingWidget::Private::_k_slotShowTaggingPopup()
{
    showTaggingPopup( QCursor::pos() );
}


Nepomuk::ResourceTaggingWidget::ResourceTaggingWidget( QWidget* parent )
    : QWidget( parent ),
      d( new Private() )
{
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setMargin( 0 );
    d->resourceTagCloud = new TagCloud( this );
    layout->addWidget( d->resourceTagCloud );

    d->changeTagsAction = new QAction( i18n( "Change tags..." ), this );
    d->resourceTagCloud->setCustomNewTagAction( d->changeTagsAction );

    // the popup tag cloud
    d->popup = new TaggingPopup;
    d->popup->setSelectionEnabled( true );
    d->popup->setNewTagButtonEnabled( true );

    connect( d->popup, SIGNAL( tagToggled( const Nepomuk::Tag&, bool ) ),
             this, SLOT( slotTagToggled( const Nepomuk::Tag&, bool ) ) );
    connect( d->popup, SIGNAL( tagAdded( const Nepomuk::Tag& ) ),
             this, SLOT( slotTagAdded( const Nepomuk::Tag& ) ) );

    connect( d->changeTagsAction, SIGNAL( activated() ),
             this, SLOT( _k_slotShowTaggingPopup() ) );

    connect( d->resourceTagCloud, SIGNAL( tagClicked( const Nepomuk::Tag& ) ),
             this, SIGNAL( tagClicked( const Nepomuk::Tag& ) ) );
}


Nepomuk::ResourceTaggingWidget::~ResourceTaggingWidget()
{
    delete d->popup;
    delete d;
}


void Nepomuk::ResourceTaggingWidget::setResource( const Nepomuk::Resource& res )
{
    d->resource = res;
    d->resourceTagCloud->showResourceTags( res );
}


void Nepomuk::ResourceTaggingWidget::slotTagToggled( const Nepomuk::Tag& tag, bool enabled )
{
    if ( enabled ) {
        d->resourceTags.append( tag );
    }
    else {
        d->resourceTags.removeAll( tag );
    }
    d->popup->hide();
}


void Nepomuk::ResourceTaggingWidget::slotTagAdded( const Nepomuk::Tag& tag )
{
    // assign it right away
    d->resourceTags.append( tag );
    d->resource.addTag( tag );
}


void Nepomuk::ResourceTaggingWidget::contextMenuEvent( QContextMenuEvent* e )
{
    d->showTaggingPopup( e->globalPos() );
}

#include "resourcetaggingwidget.moc"
