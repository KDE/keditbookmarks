/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 * Copyright (c) 2000 Daniel Molkentin <molkentin@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <unistd.h>

#include <kconfig.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kgenericfactory.h>
#include <qtabwidget.h>
#include <qlayout.h>

#include "jsopts.h"
#include "javaopts.h"
#include "pluginopts.h"
#include "appearance.h"
#include "htmlopts.h"

#include "main.h"
#include <kaboutdata.h>
#include "main.moc"

extern "C"
{
	KCModule *create_khtml_behavior(QWidget *parent, const char *name)
	{
		KConfig *c = new KConfig( "konquerorrc", false, false );
		return new KMiscHTMLOptions(c, "HTML Settings", parent, name);
	}

	KCModule *create_khtml_fonts(QWidget *parent, const char *name)
	{
		KConfig *c = new KConfig( "konquerorrc", false, false );
		return new KAppearanceOptions(c, "HTML Settings", parent, name);
	}

	KCModule *create_khtml_java_js(QWidget *parent, const char */*name*/)
	{
		KConfig *c = new KConfig( "konquerorrc", false, false );
		return new KJSParts(c, parent, "kcmkonqhtml");
	}

	KCModule *create_khtml_plugins(QWidget *parent, const char *name)
	{
		KConfig *c = new KConfig( "konquerorrc", false, false );
		return new KPluginOptions(c, "Java/JavaScript Settings", parent, name);
	}


}


KJSParts::KJSParts(KConfig *config, QWidget *parent, const char *name)
	: KCModule(parent, name), mConfig(config)
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  tab = new QTabWidget(this);
  layout->addWidget(tab);

  java = new KJavaOptions( config, "Java/JavaScript Settings", this, name );
  tab->addTab( java, i18n( "&Java" ) );
  connect( java, SIGNAL( changed( bool ) ), SIGNAL( changed( bool ) ) );

  javascript = new KJavaScriptOptions( config, "Java/JavaScript Settings", this, name );
  tab->addTab( javascript, i18n( "Java&Script" ) );
  connect( javascript, SIGNAL( changed( bool ) ), SIGNAL( changed( bool ) ) );
}

KJSParts::~KJSParts()
{
  delete mConfig;
}

void KJSParts::load()
{
  javascript->load();
  java->load();
}


void KJSParts::save()
{
  javascript->save();
  java->save();

  // Send signal to konqueror
  // Warning. In case something is added/changed here, keep kfmclient in sync
  QByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );
}


void KJSParts::defaults()
{
  javascript->defaults();
  java->defaults();
}

QString KJSParts::quickHelp() const
{
  return i18n("<h1>Konqueror Browser</h1> Here you can configure Konqueror's browser "
              "functionality. Please note that the file manager "
              "functionality has to be configured using the \"File Manager\" "
              "configuration module."
              "<h2>HTML</h2>On the HTML page, you can make some "
              "settings how Konqueror should handle the HTML code in "
              "the web pages it loads. It is usually not necessary to "
              "change anything here."
              "<h2>Appearance</h2>On this page, you can configure "
              "which fonts Konqueror should use to display the web "
              "pages you view."
              "<h2>JavaScript</h2>On this page, you can configure "
              "whether JavaScript programs embedded in web pages should "
              "be allowed to be executed by Konqueror."
              "<h2>Java</h2>On this page, you can configure "
              "whether Java applets embedded in web pages should "
              "be allowed to be executed by Konqueror."
              "<br><br><b>Note:</b> Active content is always a "
              "security risk, which is why Konqueror allows you to specify very "
              "fine-grained from which hosts you want to execute Java and/or "
              "JavaScript programs." );
}

const KAboutData* KJSParts::aboutData() const
{
    KAboutData *about =
    new KAboutData(I18N_NOOP("kcmkonqhtml"), I18N_NOOP("Konqueror Browsing Control Module"),
                  0, 0, KAboutData::License_GPL,
                  I18N_NOOP("(c) 1999 - 2001 The Konqueror Developers"));

    about->addAuthor("Waldo Bastian",0,"bastian@kde.org");
    about->addAuthor("David Faure",0,"faure@kde.org");
    about->addAuthor("Matthias Kalle Dalheimer",0,"kalle@kde.org");
    about->addAuthor("Lars Knoll",0,"knoll@kde.org");
    about->addAuthor("Dirk Mueller",0,"mueller@kde.org");
    about->addAuthor("Daniel Molkentin",0,"molkentin@kde.org");
    about->addAuthor("Wynn Wilkes",0,"wynnw@caldera.com");


    return about;
}

