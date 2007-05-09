/* This file is part of the KDE project
   Copyright (c) 2005 Pascal Létourneau <pascal.letourneau@kdemail.net>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_iconviewitemdelegate.h"

#include <QtCore/QModelIndex>
#include <QtGui/QPainter>

KonqIconViewItemDelegate::KonqIconViewItemDelegate( QObject* parent ) : QItemDelegate( parent )
{
}

KonqIconViewItemDelegate::~KonqIconViewItemDelegate()
{
}

void KonqIconViewItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItem opt = option;
    if ( opt.decorationPosition == QStyleOptionViewItem::Top )
        opt.displayAlignment = Qt::AlignHCenter;
    else
        opt.displayAlignment = Qt::AlignLeft;
 
    QItemDelegate::paint( painter, opt, index );
}

void KonqIconViewItemDelegate::drawDisplay( QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QString& text ) const
{
    QItemDelegate::drawDisplay( painter, option, rect, text );
}

void KonqIconViewItemDelegate::drawFocus( QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect ) const
{
    QItemDelegate::drawFocus( painter, option, rect );
}


#include "konq_iconviewitemdelegate.moc"
