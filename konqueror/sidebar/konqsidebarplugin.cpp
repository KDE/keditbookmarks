/* This file is part of the KDE project
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

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

#include "konqsidebarplugin.moc"
#include <kdebug.h>


KonqSidebarPlugin::KonqSidebarPlugin(KInstance *instance,QObject *parent,QWidget * /*widgetParent*/,QString &desktopName_, const char* name):QObject(parent,name),desktopName(desktopName_){m_parentInstance=instance;}

KonqSidebarPlugin::~KonqSidebarPlugin(){;}

KInstance *KonqSidebarPlugin::parentInstance(){return m_parentInstance;}

void KonqSidebarPlugin::openURL(const KURL& url){handleURL(url);}

void KonqSidebarPlugin::openPreview(const KFileItemList& items)
{
  handlePreview(items);
}

void KonqSidebarPlugin::openPreviewOnMouseOver(const KFileItem& item)
{
  handlePreviewOnMouseOver(item);
}

void KonqSidebarPlugin::handlePreview(const KFileItemList & /*items*/) {}

void KonqSidebarPlugin::handlePreviewOnMouseOver(const KFileItem& /*items*/) {}

