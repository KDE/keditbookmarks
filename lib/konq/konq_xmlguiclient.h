/* This file is part of the KDE project
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>

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

#ifndef __konqxmlguiclient_h
#define __konqxmlguiclient_h

#include <sys/types.h>

#include <kaction.h>
#include <kxmlguiclient.h>
#include <qstringlist.h>
/** This class implements common methods to manipulate the DOMDocument of KXMLGUIClient
	*
	*/
class KonqXMLGUIClient : public KXMLGUIClient
{
public:
  KonqXMLGUIClient( );
  KonqXMLGUIClient( KXMLGUIClient *parent );
  virtual ~KonqXMLGUIClient( );
  /**
   * Reimplemented for internal purpose
   */
  QDomDocument domDocument( ) const;

  QDomElement DomElement( ) const;

protected:
  void addAction( KAction *action, const QDomElement &menu = QDomElement() );
  void addAction( const char *name, const QDomElement &menu = QDomElement() );
  void addSeparator( const QDomElement &menu = QDomElement() );
  void addGroup( const QString &grp );
  void addMerge( const QString &name );

  void prepareXMLGUIStuff();
  QDomDocument m_doc;
  QDomElement m_menuElement;
	QString attrName;

};
#endif

