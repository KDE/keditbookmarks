/* This file is part of the KDE project
   Copyright (C) 2003, George Staikos <staikos@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "web_module.h"
#include <qfileinfo.h>
#include <qtimer.h>
#include <qspinbox.h>
#include <qhbox.h>

#include <kdialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <dom/html_inline.h>
#include <konq_pixmapprovider.h>


KonqSideBarWebModule::KonqSideBarWebModule(KInstance *instance, QObject *parent, QWidget *widgetParent, QString &desktopName, const char* name)
	: KonqSidebarPlugin(instance, parent, widgetParent, desktopName, name)
{
	_htmlPart = new KHTMLSideBar(universalMode());
	connect(_htmlPart, SIGNAL(reload()), this, SLOT(reload()));
	connect(_htmlPart, SIGNAL(completed()), this, SLOT(pageLoaded()));
	connect(_htmlPart,
		SIGNAL(setWindowCaption(const QString&)),
		this,
		SLOT(setTitle(const QString&)));
	connect(_htmlPart,
		SIGNAL(openURLRequest(const QString&, KParts::URLArgs)),
		this,
		SLOT(urlClicked(const QString&, KParts::URLArgs)));
	connect(_htmlPart,
		SIGNAL(setAutoReload()), this, SLOT( setAutoReload() ));
	connect(_htmlPart,
		SIGNAL(openURLNewWindow(const QString&, KParts::URLArgs)),
		this,
		SLOT(urlNewWindow(const QString&, KParts::URLArgs)));
	connect(_htmlPart,
		SIGNAL(submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&)),
		this,
		SIGNAL(submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&)));

	_desktopName = desktopName;

	KSimpleConfig ksc(_desktopName);
	ksc.setGroup("Desktop Entry");
        reloadTimeout = ksc.readNumEntry("Reload", 0);
	_url = ksc.readPathEntry("URL");
	_htmlPart->openURL(_url );
	// Must load this delayed
	QTimer::singleShot(0, this, SLOT(loadFavicon()));
}


KonqSideBarWebModule::~KonqSideBarWebModule() {
	delete _htmlPart;
	_htmlPart = 0L;
}


QWidget *KonqSideBarWebModule::getWidget() {
	return _htmlPart->widget();
}

void KonqSideBarWebModule::setAutoReload(){
	KDialogBase dlg(0, "", true, i18n("Set Refresh Timeout (0 disables)"),
			KDialogBase::Ok|KDialogBase::Cancel);
	QHBox *hbox = dlg.makeHBoxMainWidget();
	
	QSpinBox *mins = new QSpinBox( 0, 120, 1, hbox );
	mins->setSuffix( i18n(" minutes") );
	QSpinBox *secs = new QSpinBox( 0, 59, 1, hbox );
	secs->setSuffix( i18n(" seconds") );

	if( reloadTimeout > 0 )	{
		int seconds = reloadTimeout / 1000;
		secs->setValue( seconds % 60 );
		mins->setValue( ( seconds - secs->value() ) / 60 );
	}
	
	if( dlg.exec() == QDialog::Accepted ) {
		int msec = ( mins->value() * 60 + secs->value() ) * 1000;
		reloadTimeout = msec;
		KSimpleConfig ksc(_desktopName);
		ksc.setGroup("Desktop Entry");
		ksc.writeEntry("Reload", reloadTimeout);	
		reload();
	}
}

void *KonqSideBarWebModule::provides(const QString &) {
	return 0L;
}


void KonqSideBarWebModule::handleURL(const KURL &) {
}


void KonqSideBarWebModule::urlNewWindow(const QString& url, KParts::URLArgs args)
{
	emit createNewWindow(KURL( url ), args);
}


void KonqSideBarWebModule::urlClicked(const QString& url, KParts::URLArgs args) 
{
	emit openURLRequest( KURL(url), args);
}


void KonqSideBarWebModule::loadFavicon() {
	QString icon = KonqPixmapProvider::iconForURL(_url.url());
	if (icon.isEmpty()) {
		KonqFavIconMgr::downloadHostIcon(_url);
		icon = KonqPixmapProvider::iconForURL(_url.url());
	}

	if (!icon.isEmpty()) {
		emit setIcon(icon);

		KSimpleConfig ksc(_desktopName);
		ksc.setGroup("Desktop Entry");
		if (icon != ksc.readPathEntry("Icon")) {
			ksc.writePathEntry("Icon", icon);
		}
	}
}


void KonqSideBarWebModule::reload() {
	_htmlPart->openURL(_url);
}


void KonqSideBarWebModule::setTitle(const QString& title) {
	if (!title.isEmpty()) {
		emit setCaption(title);

		KSimpleConfig ksc(_desktopName);
		ksc.setGroup("Desktop Entry");
		if (title != ksc.readPathEntry("Name")) {
			ksc.writePathEntry("Name", title);
		}
	}
}


void KonqSideBarWebModule::pageLoaded() {
	if( reloadTimeout > 0 ) {
		QTimer::singleShot( reloadTimeout, this, SLOT( reload() ) );
	}
}


extern "C" {
	KonqSidebarPlugin* create_konqsidebar_web(KInstance *instance, QObject *parent, QWidget *widget, QString &desktopName, const char *name) {
		return new KonqSideBarWebModule(instance, parent, widget, desktopName, name);
	}
}


extern "C" {
	bool add_konqsidebar_web(QString* fn, QString* param, QMap<QString,QString> *map) {
		Q_UNUSED(param);
		KGlobal::dirs()->addResourceType("websidebardata", KStandardDirs::kde_default("data") + "konqsidebartng/websidebar");
		KURL url;
		url.setProtocol("file");
		QStringList paths = KGlobal::dirs()->resourceDirs("websidebardata");
		for (QStringList::Iterator i = paths.begin(); i != paths.end(); ++i) {
			if (QFileInfo(*i + "websidebar.html").exists()) {
				url.setPath(*i + "websidebar.html");
				break;
			}
		}

		if (url.path().isEmpty())
			return false;
		map->insert("Type", "Link");
		map->insert("URL", url.url());
		map->insert("Icon", "netscape");
		map->insert("Name", i18n("Web SideBar Plugin"));
		map->insert("Open", "true");
		map->insert("X-KDE-KonqSidebarModule","konqsidebar_web");
		fn->setLatin1("websidebarplugin%1.desktop");
		return true;
	}
}


#include "web_module.moc"

