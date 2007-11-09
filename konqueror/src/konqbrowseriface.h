/* This file is part of the KDE project
   Copyright (C) 2001 Simon Hausmann <hausmann@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_browseriface_h__
#define __konq_browseriface_h__

#include <kparts/browserinterface.h>

class KonqView;

class KonqBrowserInterface : public KParts::BrowserInterface
{
    Q_OBJECT
    Q_PROPERTY( uint historyLength READ historyLength )
public:
    explicit KonqBrowserInterface( KonqView *view );

    uint historyLength() const;

public Q_SLOTS:
    void goHistory( int );

private:
    KonqView *m_view;
};

#endif
