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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <qfile.h>

#include <kcmoduleloader.h>
#include <klocale.h>

#include "kcookiesmain.h"
#include "netpref.h"
#include "smbrodlg.h"
#include "useragentdlg.h"
#include "kproxydlg.h"
#include "cache.h"

#include "main.h"

extern "C"
{

  KCModule *create_cookie(QWidget *parent, const char /**name*/)
  {
    return new KCookiesMain(parent);
  }

  KCModule *create_smb(QWidget *parent, const char /**name*/)
  {
    return new SMBRoOptions(parent);
  }

  KCModule *create_useragent(QWidget *parent, const char /**name*/)
  {
    return new UserAgentDlg(parent);
  }

  KCModule *create_proxy(QWidget *parent, const char /**name*/)
  {
    return new KProxyOptions(parent);
  }

  KCModule *create_cache(QWidget *parent, const char /**name*/)
  {
    return new KCacheConfigDialog( parent );
  }

  KCModule *create_netpref(QWidget *parent, const char /**name*/)
  {
    return new KIOPreferences(parent);
  }

  KCModule *create_lanbrowser(QWidget *parent, const char *)
  {
    return new LanBrowser(parent);
  }

}

LanBrowser::LanBrowser(QWidget *parent)
:KCModule(parent,"kcmkio")
,layout(this)
,tabs(this)
{
   layout.addWidget(&tabs);

   smbPage = create_smb(&tabs, 0);
   tabs.addTab(smbPage, i18n("&Windows Shares"));
   connect(smbPage,SIGNAL(changed(bool)),this,SLOT(slotEmitChanged()));

   lisaPage = KCModuleLoader::loadModule("kcmlisa", &tabs);
   if (lisaPage)
   {
     tabs.addTab(lisaPage,i18n("&LISa Daemon"));
     connect(lisaPage,SIGNAL(changed()),this,SLOT(slotEmitChanged()));
   }

//   resLisaPage = KCModuleLoader::loadModule("kcmreslisa", &tabs);
//   if (resLisaPage)
//   {
//     tabs.addTab(resLisaPage,i18n("R&esLISa Daemon"));
//     connect(resLisaPage,SIGNAL(changed()),this,SLOT(slotEmitChanged()));
//   }

   kioLanPage = KCModuleLoader::loadModule("kcmkiolan", &tabs);
   if (kioLanPage)
   {
     tabs.addTab(kioLanPage,i18n("lan:/ Iosla&ve"));
     connect(kioLanPage,SIGNAL(changed()),this,SLOT(slotEmitChanged()));
   }

   setButtons(Apply|Help);
   load();
}

void LanBrowser::slotEmitChanged()
{
   emit changed(true);
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


QString LanBrowser::quickHelp() const
{
   return i18n("<h1>Local Network Browsing</h1>Here you setup your "
		"<b>\"Network Neighborhood\"</b>. You "
		"can use either the LISa daemon and the lan:/ ioslave, or the "
		"ResLISa daemon and the rlan:/ ioslave.<br><br>"
		"About the <b>LAN ioslave</b> configuration:<br> If you select it, the "
		"ioslave, <i>if available</i>, will check whether the host "
		"supports this service when you open this host. Please note "
		"that paranoid people might consider even this to be an attack.<br>"
		"<i>Always</i> means that you will always see the links for the "
		"services, regardless of whether they are actually offered by the host. "
		"<i>Never</i> means that you will never have the links to the services. "
		"In both cases you will not contact the host, so nobody will ever regard "
		"you as an attacker.<br><br>More information about <b>LISa</b> "
		"can be found at <a href=\"http://lisa-home.sourceforge.net\">"
		"the LISa Homepage</a> or contact Alexander Neundorf "
		"&lt;<a href=\"mailto:neundorf@kde.org\">neundorf@kde.org</a>&gt;.");
}

#include "main.moc"

