/*    kcookiesmain.cpp - Cookies configuration
 *
 *    First version of cookies configuration:
 *        Copyright (C) Waldo Bastian <bastian@kde.org>
 *    This dialog box:
 *        Copyright (C) David Faure <faure@kde.org>
 *
 */

// Own
#include "kcookiesmain.h"

// Qt
#include <QtGui/QLayout>
#include <QtGui/QTabWidget>
#include <QtDBus/QtDBus>

// KDE
#include <klocale.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kgenericfactory.h>

// Local
#include "kcookiespolicies.h"
#include "kcookiesmanagement.h"

K_PLUGIN_FACTORY_DECLARATION(KioConfigFactory)

KCookiesMain::KCookiesMain(QWidget *parent, const QVariantList &)
  : KCModule(KioConfigFactory::componentData(), parent)
{
    management = 0;
    bool managerOK = true;

    QDBusInterface kded("org.kde.kded", "/kded", "org.kde.kded");
    QDBusReply<bool> reply = kded.call("loadModule",QString( "kcookiejar" ) );

    if( !reply.isValid() )
    {
       managerOK = false;
       kDebug(7103) << "kcm_kio: KDED could not load KCookiejar!";
       KMessageBox::sorry(0, i18n("Unable to start the cookie handler service.\n"
                             "You will not be able to manage the cookies that "
                             "are stored on your computer."));
    }

    QVBoxLayout *layout = new QVBoxLayout(this);
    tab = new QTabWidget(this);
    layout->addWidget(tab);

    policies = new KCookiesPolicies(componentData(), this);
    tab->addTab(policies, i18n("&Policy"));
    connect(policies, SIGNAL(changed(bool)), SIGNAL(changed(bool)));

    if( managerOK )
    {
        management = new KCookiesManagement(componentData(), this);
        tab->addTab(management, i18n("&Management"));
        connect(management, SIGNAL(changed(bool)), SIGNAL(changed(bool)));
    }
}

KCookiesMain::~KCookiesMain()
{
}

void KCookiesMain::save()
{
  policies->save();
  if ( management )
      management->save();
}

void KCookiesMain::defaults()
{
  KCModule* module = static_cast<KCModule*>(tab->currentWidget());

  if ( module == policies )
    policies->defaults();
  else if( management )
    management->defaults();
}

QString KCookiesMain::quickHelp() const
{
  return i18n("<p><h1>Cookies</h1> Cookies contain information that Konqueror"
    " (or other KDE applications using the HTTP protocol) stores on your"
    " computer, initiated by a remote Internet server. This means that"
    " a web server can store information about you and your browsing activities"
    " on your machine for later use. You might consider this an invasion of"
    " privacy.</p><p> However, cookies are useful in certain situations. For example, they"
    " are often used by Internet shops, so you can 'put things into a shopping basket'."
    " Some sites require you have a browser that supports cookies.</p><p>"
    " Because most people want a compromise between privacy and the benefits cookies offer,"
    " KDE offers you the ability to customize the way it handles cookies. So you might want"
    " to set KDE's default policy to ask you whenever a server wants to set a cookie,"
    " allowing you to decide. For your favorite shopping web sites that you trust, you might"
    " want to set the policy to accept, then you can access the web sites without being prompted"
    " every time KDE receives a cookie.</p>" );
}

#include "kcookiesmain.moc"
