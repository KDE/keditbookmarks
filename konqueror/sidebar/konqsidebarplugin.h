/* This file is part of the KDE project
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>

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
#ifndef _konqsidebarplugin_h_
#define _konqsidebarplugin_h_
#include <qwidget.h>
#include <qobject.h>
#include <kurl.h>
#include <qstring.h>
#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kio/job.h>
#include <qguardedptr.h>
#include <kfileitem.h>

class KonqSidebarPluginPrivate;

class KonqSidebarPlugin : public QObject
{
	Q_OBJECT
	public:
		KonqSidebarPlugin(KInstance *instance,QObject *parent,QWidget *widgetParent,const QString &desktopName_, const char* name=0);
		~KonqSidebarPlugin();
		virtual QWidget *getWidget()=0;
		virtual void *provides(const QString &)=0;
		KInstance *parentInstance();
	protected:
		virtual void handleURL(const KURL &url)=0;
		virtual void handlePreview(const KFileItemList & items);
		virtual void handlePreviewOnMouseOver(const KFileItem &items); //not used yet, perhaps in KDE 3.1
		QString desktopName;
		KInstance* m_parentInstance;

	private:
		KonqSidebarPluginPrivate *d;

	signals:
		void requestURL(KURL&);
		void started(KIO::Job *);
		void completed();

	public slots:
	  void openURL(const KURL& url);

	  void openPreview(const KFileItemList& items);

	  void openPreviewOnMouseOver(const KFileItem& item); // not used yet, perhaps KDE 3.1
	/*
		if your plugin supports a setup dialog, instead (replaces the url menu entry in the popup) (not supported yet)
			void setup(QWidget *parent);

	 */


	/* signals, which could be, but need not to be added

		void openURLRequest( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
  		void createNewWindow( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );

		void enableAction( const char * name, bool enabled );

		void popupMenu( const QPoint &global, const KFileItemList &items );
  		void popupMenu( KXMLGUIClient *client, const QPoint &global, const KFileItemList &items );
		void popupMenu( const QPoint &global, const KURL &url,
			const QString &mimeType, mode_t mode = (mode_t)-1 );
		void popupMenu( KXMLGUIClient *client,
			const QPoint &global, const KURL &url,
			const QString &mimeType, mode_t mode = (mode_t)-1 );

		void showError(QString &);	//for later extension
		void showMessage(QString &);	//for later extension

	*/

};

#endif
