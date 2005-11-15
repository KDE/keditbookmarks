/**
 * kcookiesmanagement.cpp - Cookies manager
 *
 * Copyright 2000-2001 Marco Pinelli <pinmc@orion.it>
 * Copyright (c) 2000-2001 Dawit Alemayehu <adawit@kde.org>
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

#include <qapplication.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <q3groupbox.h>
#include <q3hbox.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qtoolbutton.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QList>
#include <Q3CString>

#include <kidna.h>
#include <kdebug.h>
#include <klocale.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klistview.h>
#include <klistviewsearchline.h>
#include <kmessagebox.h>
#include <dcopref.h>

#include "kcookiesmain.h"
#include "kcookiespolicies.h"
#include "kcookiesmanagement.h"
#include "kcookiesmanagementdlg_ui.h"

#include <assert.h>

struct CookieProp
{
    QString host;
    QString name;
    QString value;
    QString domain;
    QString path;
    QString expireDate;
    QString secure;
    bool allLoaded;
};

CookieListViewItem::CookieListViewItem(Q3ListView *parent, QString dom)
                   :Q3ListViewItem(parent)
{
    init( 0, dom );
}

CookieListViewItem::CookieListViewItem(Q3ListViewItem *parent, CookieProp *cookie)
                   :Q3ListViewItem(parent)
{
    init( cookie );
}

CookieListViewItem::~CookieListViewItem()
{
    delete mCookie;
}

void CookieListViewItem::init( CookieProp* cookie, QString domain,
                               bool cookieLoaded )
{
    mCookie = cookie;
    mDomain = domain;
    mCookiesLoaded = cookieLoaded;
}

CookieProp* CookieListViewItem::leaveCookie()
{
    CookieProp *ret = mCookie;
    mCookie = 0;
    return ret;
}

QString CookieListViewItem::text(int f) const
{
    if (mCookie)
        return f == 0 ? QString::null : KIDNA::toUnicode(mCookie->host);
    else
        return f == 0 ? KIDNA::toUnicode(mDomain) : QString::null;
}

KCookiesManagement::KCookiesManagement(KInstance *inst, QWidget *parent)
                   : KCModule(inst, parent)
{
  // Toplevel layout
  QVBoxLayout* mainLayout = new QVBoxLayout(this, KDialog::marginHint(),
                                            KDialog::spacingHint());

  dlg = new KCookiesManagementDlgUI (this);
  
  dlg->tbClearSearchLine->setIconSet(SmallIconSet(QApplication::isRightToLeft() ? "clear_left" : "locationbar_erase"));
  dlg->kListViewSearchLine->setListView(dlg->lvCookies);
  
  mainLayout->addWidget(dlg);
  dlg->lvCookies->setSorting(0);

  connect(dlg->lvCookies, SIGNAL(expanded(Q3ListViewItem*)), SLOT(getCookies(Q3ListViewItem*)) );
  connect(dlg->lvCookies, SIGNAL(selectionChanged(Q3ListViewItem*)), SLOT(showCookieDetails(Q3ListViewItem*)) );

  connect(dlg->pbDelete, SIGNAL(clicked()), SLOT(deleteCookie()));
  connect(dlg->pbDeleteAll, SIGNAL(clicked()), SLOT(deleteAllCookies()));
  connect(dlg->pbReload, SIGNAL(clicked()), SLOT(getDomains()));
  connect(dlg->pbPolicy, SIGNAL(clicked()), SLOT(doPolicy()));

  connect(dlg->lvCookies, SIGNAL(doubleClicked (Q3ListViewItem *)), SLOT(doPolicy()));
  deletedCookies.setAutoDelete(true);
  m_bDeleteAll = false;
  mainWidget = parent;

  load();
}

KCookiesManagement::~KCookiesManagement()
{
}

void KCookiesManagement::load()
{
  reset();
  getDomains();
}

void KCookiesManagement::save()
{
  // If delete all cookies was requested!
  if(m_bDeleteAll)
  {
    if(!DCOPRef("kded", "kcookiejar").send("deleteAllCookies"))
    {
      QString caption = i18n ("DCOP Communication Error");
      QString message = i18n ("Unable to delete all the cookies as requested.");
      KMessageBox::sorry (this, caption, message);
      return;
    }
    
    m_bDeleteAll = false; // deleted[Cookies|Domains] have been cleared yet    
  }

  // Certain groups of cookies were deleted...
  QStringList::Iterator dIt = deletedDomains.begin();
  while( dIt != deletedDomains.end() )
  {
    QByteArray call;
    QByteArray reply;
    Q3CString replyType;
    QDataStream callStream(&call, QIODevice::WriteOnly);

    callStream.setVersion(QDataStream::Qt_3_1);
    callStream << *dIt;

    if( !DCOPRef("kded", "kcookiejar").send("deleteCookiesFromDomain", (*dIt)) )
    {
      QString caption = i18n ("DCOP Communication Error");
      QString message = i18n ("Unable to delete cookies as requested.");
      KMessageBox::sorry (this, caption, message);
      return;
    }
    
    dIt = deletedDomains.remove(dIt);
  }

  // Individual cookies were deleted...
  bool success = true; // Maybe we can go on...
  Q3DictIterator<CookiePropList> cookiesDom(deletedCookies);
  
  while(cookiesDom.current())
  {
    CookiePropList *list = cookiesDom.current();
    Q3PtrListIterator<CookieProp> cookie(*list);

    while(*cookie)
    {
      if( !DCOPRef("kded", "kcookiejar").send("deleteCookie",(*cookie)->domain,
                                              (*cookie)->host, (*cookie)->path,
                                              (*cookie)->name) )
      {
        success = false;
        break;
      }
      
      list->removeRef(*cookie);
    }

    if(!success)
      break;
    
    deletedCookies.remove(cookiesDom.currentKey());
  }
  
  emit changed( false );
}

void KCookiesManagement::defaults()
{
  reset();
  getDomains();
  emit changed (false);
}

void KCookiesManagement::reset(bool deleteAll)
{
  if ( !deleteAll )
    m_bDeleteAll = false;
  
  clearCookieDetails();
  dlg->lvCookies->clear();
  deletedDomains.clear();
  deletedCookies.clear();
  dlg->pbDelete->setEnabled(false);
  dlg->pbDeleteAll->setEnabled(false);
  dlg->pbPolicy->setEnabled(false);
}

void KCookiesManagement::clearCookieDetails()
{
  dlg->leName->clear();
  dlg->leValue->clear();
  dlg->leDomain->clear();
  dlg->lePath->clear();
  dlg->leExpires->clear();
  dlg->leSecure->clear();
}

QString KCookiesManagement::quickHelp() const
{
  return i18n("<h1>Cookies Management Quick Help</h1>" );
}

void KCookiesManagement::getDomains()
{
  DCOPReply reply = DCOPRef("kded", "kcookiejar").call("findDomains");

  if( !reply.isValid() )
  {
    QString caption = i18n ("Information Lookup Failure");
    QString message = i18n ("Unable to retrieve information about the "
                            "cookies stored on your computer.");
    KMessageBox::sorry (this, caption, message);
    return;
  }
  
  QStringList domains = reply;

  if ( dlg->lvCookies->childCount() )
  {
    reset();
    dlg->lvCookies->setCurrentItem( 0L );
  }

  CookieListViewItem *dom;
  for(QStringList::Iterator dIt = domains.begin(); dIt != domains.end(); dIt++)
  {
    dom = new CookieListViewItem(dlg->lvCookies, *dIt);
    dom->setExpandable(true);
  }

  // are ther any cookies?
  dlg->pbDeleteAll->setEnabled(dlg->lvCookies->childCount());
}

void KCookiesManagement::getCookies(Q3ListViewItem *cookieDom)
{
  CookieListViewItem* ckd = static_cast<CookieListViewItem*>(cookieDom);
  if ( ckd->cookiesLoaded() )
    return;

  QList<int> fields;
  fields << 0 << 1 << 2 << 3;

  DCOPReply reply = DCOPRef ("kded", "kcookiejar").call ("findCookies",
                                                         DCOPArg(fields, "QValueList<int>"),
                                                         ckd->domain(),
                                                         QString(),
                                                         QString(),
                                                         QString());
  if(reply.isValid())
  {
    QStringList fieldVal = reply;
    QStringList::Iterator fIt = fieldVal.begin();

    while(fIt != fieldVal.end())
    {
      CookieProp *details = new CookieProp;
      details->domain = *fIt++;
      details->path = *fIt++;
      details->name = *fIt++;
      details->host = *fIt++;
      details->allLoaded = false;
      new CookieListViewItem(cookieDom, details);
    }

    static_cast<CookieListViewItem*>(cookieDom)->setCookiesLoaded();
  }
}

bool KCookiesManagement::cookieDetails(CookieProp *cookie)
{
  QList<int> fields;
  fields << 4 << 5 << 7;

  DCOPReply reply = DCOPRef ("kded", "kcookiejar").call ("findCookies",
                                                         DCOPArg(fields, "QValueList<int>"),
                                                         cookie->domain,
                                                         cookie->host,
                                                         cookie->path,
                                                         cookie->name);
  if( !reply.isValid() )
    return false;

  QStringList fieldVal = reply;

  QStringList::Iterator c = fieldVal.begin();
  cookie->value = *c++;
  unsigned tmp = (*c++).toUInt();

  if( tmp == 0 )
    cookie->expireDate = i18n("End of session");
  else
  {
    QDateTime expDate;
    expDate.setTime_t(tmp);
    cookie->expireDate = KGlobal::locale()->formatDateTime(expDate);
  }

  tmp = (*c).toUInt();
  cookie->secure = i18n(tmp ? "Yes" : "No");
  cookie->allLoaded = true;
  return true;
}

void KCookiesManagement::showCookieDetails(Q3ListViewItem* item)
{
  kdDebug () << "::showCookieDetails... " << endl;
  CookieProp *cookie = static_cast<CookieListViewItem*>(item)->cookie();
  if( cookie )
  {
    if( cookie->allLoaded || cookieDetails(cookie) )
    {
      dlg->leName->validateAndSet(cookie->name,0,0,0);
      dlg->leValue->validateAndSet(cookie->value,0,0,0);
      dlg->leDomain->validateAndSet(cookie->domain,0,0,0);
      dlg->lePath->validateAndSet(cookie->path,0,0,0);
      dlg->leExpires->validateAndSet(cookie->expireDate,0,0,0);
      dlg->leSecure->validateAndSet(cookie->secure,0,0,0);
    }

    dlg->pbPolicy->setEnabled (true);
  }
  else
  {
    clearCookieDetails();
    dlg->pbPolicy->setEnabled(false);
  }

  dlg->pbDelete->setEnabled(true);
}

void KCookiesManagement::doPolicy()
{
  // Get current item
  CookieListViewItem *item = static_cast<CookieListViewItem*>( dlg->lvCookies->currentItem() );

  if( item && item->cookie())
  {
    CookieProp *cookie = item->cookie();

    QString domain = cookie->domain;

    if( domain.isEmpty() )
    {
      CookieListViewItem *parent = static_cast<CookieListViewItem*>( item->parent() );

      if ( parent )
        domain = parent->domain ();
    }

    KCookiesMain* mainDlg =static_cast<KCookiesMain*>( mainWidget );
    // must be present or something is really wrong.
    assert (mainDlg);

    KCookiesPolicies* policyDlg = mainDlg->policyDlg();
    // must be present unless someone rewrote the widget in which case
    // this needs to be re-written as well.
    assert (policyDlg);
    policyDlg->addNewPolicy(domain);
  }
}


void KCookiesManagement::deleteCookie()
{
  Q3ListViewItem* currentItem = dlg->lvCookies->currentItem();
  CookieListViewItem *item = static_cast<CookieListViewItem*>( currentItem );
  if( item->cookie() )
  {
    CookieListViewItem *parent = static_cast<CookieListViewItem*>(item->parent());
    CookiePropList *list = deletedCookies.find(parent->domain());
    if(!list)
    {
      list = new CookiePropList;
      list->setAutoDelete(true);
      deletedCookies.insert(parent->domain(), list);
    }
    list->append(item->leaveCookie());
    delete item;
    if(parent->childCount() == 0)
      delete parent;
  }
  else
  {
    deletedDomains.append(item->domain());
    delete item;
  }

  currentItem = dlg->lvCookies->currentItem();
  if ( currentItem )
  {
    dlg->lvCookies->setSelected( currentItem, true );
    showCookieDetails( currentItem );
  }
  else
    clearCookieDetails();

  dlg->pbDelete->setEnabled(dlg->lvCookies->selectedItem());
  dlg->pbDeleteAll->setEnabled(dlg->lvCookies->childCount());
  dlg->pbPolicy->setEnabled(dlg->lvCookies->selectedItem());

  emit changed( true );
}

void KCookiesManagement::deleteAllCookies()
{
  m_bDeleteAll = true;
  reset(true);

  emit changed( true );
}

#include "kcookiesmanagement.moc"
