/***************************************************************************
                               konqsidebartest.h
                             -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    copyright            : (C) 2001 Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <QtGui/QLabel>
#include <QtGui/QLayout>

class SidebarTest : public KonqSidebarPlugin
	{
		Q_OBJECT
		public:
		SidebarTest(const KComponentData &componentData,QObject *parent,QWidget *widgetParent, QString &desktopName_, const char* name=0):
                   KonqSidebarPlugin(componentData,parent,widgetParent,desktopName_,name)
		{
			widget=new QLabel("Init Value",widgetParent);			
		}
		~SidebarTest(){;}
                virtual QWidget *getWidget(){return widget;}   
		virtual void *provides(const QString &) {return 0;}  
		protected:
			QLabel *widget;
			virtual void handleURL(const KUrl &url)
				{
					widget->setText(url.url());
				}
	};

#endif
