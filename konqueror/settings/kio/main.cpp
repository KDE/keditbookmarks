// (c) Torben Weis 1998
// (c) David Faure 1998
/*
 * main.cpp for lisa,reslisa,kio_lan and kio_rlan kcm module
 *
 *  Copyright (C) 2000,2001 Alexander Neundorf <neundorf@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// Own
#include "main.h"

// Qt
#include <QtCore/QFile>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QTabWidget>

// KDE
#include <kcmoduleloader.h>
#include <klocale.h>
#include <kcomponentdata.h>
#include <kgenericfactory.h>

// Local
#include "kcookiesmain.h"
#include "netpref.h"
#include "smbrodlg.h"
#include "useragentdlg.h"
#include "kproxydlg.h"
#include "cache.h"
#include "bookmarks.h"

K_PLUGIN_FACTORY(KioConfigFactory,
        registerPlugin<LanBrowser>("lanbrowser");
        registerPlugin<UserAgentDlg>("useragent");
        registerPlugin<SMBRoOptions>("smb");
        registerPlugin<KIOPreferences>("netpref");
        registerPlugin<KProxyDialog>("proxy");
        registerPlugin<KCookiesMain>("cookie");
        registerPlugin<CacheConfigModule>("cache");
        registerPlugin<BookmarksConfigModule>("bookmarks");
	)
K_EXPORT_PLUGIN(KioConfigFactory("kcmkio"))

LanBrowser::LanBrowser(QWidget *parent, const QVariantList &)
    : KCModule(KioConfigFactory::componentData(), parent)
    , layout(this)
    , tabs(this)
{
   setQuickHelp( i18n("<h1>Local Network Browsing</h1>Here you setup your "
		"<b>\"Network Neighborhood\"</b>. You "
		"can use either the LISa daemon and the lan:/ ioslave, or the "
		"ResLISa daemon and the rlan:/ ioslave.<br /><br />"
		"About the <b>LAN ioslave</b> configuration:<br /> If you select it, the "
		"ioslave, <i>if available</i>, will check whether the host "
		"supports this service when you open this host. Please note "
		"that paranoid people might consider even this to be an attack.<br />"
		"<i>Always</i> means that you will always see the links for the "
		"services, regardless of whether they are actually offered by the host. "
		"<i>Never</i> means that you will never have the links to the services. "
		"In both cases you will not contact the host, so nobody will ever regard "
		"you as an attacker.<br /><br />More information about <b>LISa</b> "
		"can be found at <a href=\"http://lisa-home.sourceforge.net\">"
		"the LISa Homepage</a> or contact Alexander Neundorf "
		"&lt;<a href=\"mailto:neundorf@kde.org\">neundorf@kde.org</a>&gt;."));
   
   layout.setMargin(0);
   layout.addWidget(&tabs);

   smbPage = new SMBRoOptions(&tabs, QVariantList(), componentData());
   tabs.addTab(smbPage, i18n("&Windows Shares"));
   connect(smbPage,SIGNAL(changed(bool)), SIGNAL( changed(bool) ));

   lisaPage = KCModuleLoader::loadModule("kcmlisa", KCModuleLoader::None,&tabs);
   if (lisaPage)
   {
     tabs.addTab(lisaPage,i18n("&LISa Daemon"));
     connect(lisaPage,SIGNAL(changed()), SLOT( changed() ));
   }

//   resLisaPage = KCModuleLoader::loadModule("kcmreslisa", KCModuleLoader::None,&tabs);
//   if (resLisaPage)
//   {
//     tabs.addTab(resLisaPage,i18n("R&esLISa Daemon"));
//     connect(resLisaPage,SIGNAL(changed()), SLOT( changed() ));
//   }

   kioLanPage = KCModuleLoader::loadModule("kcmkiolan", KCModuleLoader::None, &tabs);
   if (kioLanPage)
   {
     tabs.addTab(kioLanPage,i18n("lan:/ Iosla&ve"));
     connect(kioLanPage,SIGNAL(changed()), SLOT( changed() ));
   }

   setButtons(Apply|Help);
}

void LanBrowser::load()
{
   smbPage->load();
   if (lisaPage)
     lisaPage->load();
//   if (resLisaPage)
//     resLisaPage->load();
   if (kioLanPage)
     kioLanPage->load();
   emit changed(false);
}

void LanBrowser::save()
{
   smbPage->save();
//   if (resLisaPage)
//     resLisaPage->save();
   if (kioLanPage)
     kioLanPage->save();
   if (lisaPage)
     lisaPage->save();
   emit changed(false);
}

#include "main.moc"

