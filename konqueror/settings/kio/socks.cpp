/**
 * socks.cpp
 *
 * Copyright (c) 2001 George Staikos <staikos@kde.org>
 * Copyright (c) 2001 Daniel Molkentin <molkentin@kde.org> (designer port)
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


#include <qlayout.h>
#include <qlabel.h>
#include <qvbuttongroup.h>
#include <qcheckbox.h>

#include <kfiledialog.h>
#include <klistview.h>
#include <kmessagebox.h>
#include <ksocks.h>
#include <kapplication.h>

#include "socks.h"
#include <kaboutdata.h>

KSocksConfig::KSocksConfig(QWidget *parent)
  : KCModule(parent, "kcmkio")
{

  QVBoxLayout *layout = new QVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());
  base = new SocksBase(this);
  layout->add(base);

  connect(base->_c_enableSocks, SIGNAL(clicked()), this, SLOT(enableChanged()));
  connect(base->bg, SIGNAL(clicked(int)), this, SLOT(methodChanged(int)));

  // The custom library
  connect(base->_c_customPath, SIGNAL(openFileDialog(KURLRequester *)), this, SLOT(chooseCustomLib(KURLRequester *)));
  connect(base->_c_customPath, SIGNAL(textChanged(const QString&)),
                     this, SLOT(customPathChanged(const QString&)));

  // Additional libpaths
  connect(base->_c_newPath, SIGNAL(openFileDialog(KURLRequester *)), this, SLOT(chooseCustomLib(KURLRequester *)));
  connect(base->_c_newPath, SIGNAL(returnPressed(const QString&)),
          this, SLOT(addThisLibrary(const QString&)));
  connect(base->_c_newPath, SIGNAL(textChanged(const QString&)),
          this, SLOT(libTextChanged(const QString&)));
  connect(base->_c_add, SIGNAL(clicked()), this, SLOT(addLibrary()));
  connect(base->_c_remove, SIGNAL(clicked()), this, SLOT(removeLibrary()));
  connect(base->_c_libs, SIGNAL(selectionChanged()), this, SLOT(libSelection()));

  // The "Test" button
  connect(base->_c_test, SIGNAL(clicked()), this, SLOT(testClicked()));

  // The config backend
  load();
}

KSocksConfig::~KSocksConfig()
{
}

void KSocksConfig::configChanged()
{
    emit changed(true);
}

void KSocksConfig::enableChanged()
{
  KMessageBox::information(NULL,
                           i18n("These changes will only apply to newly "
                                "started applications."),
                           i18n("SOCKS Support"),
                           "SOCKSdontshowagain");
  emit changed(true);
}


void KSocksConfig::methodChanged(int id)
{
  if (id == 4) {
    base->_c_customLabel->setEnabled(true);
    base->_c_customPath->setEnabled(true);
  } else {
    base->_c_customLabel->setEnabled(false);
    base->_c_customPath->setEnabled(false);
  }
  emit changed(true);
}


void KSocksConfig::customPathChanged(const QString&)
{
  emit changed(true);
}


void KSocksConfig::testClicked()
{
  save();   // we have to save before we can test!  Perhaps
            // it would be best to warn, though.

  if (KSocks::self()->hasSocks()) {
     KMessageBox::information(NULL,
                              i18n("Success! SOCKS was found and initialized."),
                              i18n("SOCKS Support"));
     // Eventually we could actually attempt to connect to a site here.
  } else {
      KMessageBox::information(NULL,
                               i18n("SOCKS could not be loaded."),
                               i18n("SOCKS Support"));
  }

  KSocks::self()->die();

}


void KSocksConfig::chooseCustomLib(KURLRequester * url)
{
  url->setMode( KFile::Directory );
/*  QString newFile = KFileDialog::getOpenFileName();
  if (newFile.length() > 0) {
    base->_c_customPath->setURL(newFile);
    emit changed(true);
  }*/
}



void KSocksConfig::libTextChanged(const QString& lib)
{
   if (lib.length() > 0)
     base-> _c_add->setEnabled(true);
   else base->_c_add->setEnabled(false);
}


void KSocksConfig::addThisLibrary(const QString& lib)
{
   if (lib.length() > 0) {
      new QListViewItem(base->_c_libs, lib);
      base->_c_newPath->clear();
      base->_c_add->setEnabled(false);
      base->_c_newPath->setFocus();
      emit changed(true);
   }
}


void KSocksConfig::addLibrary()
{
   addThisLibrary(base->_c_newPath->url());
}


void KSocksConfig::removeLibrary()
{
 QListViewItem *thisitem = base->_c_libs->selectedItem();
   base->_c_libs->takeItem(thisitem);
   delete thisitem;
   base->_c_libs->clearSelection();
   base->_c_remove->setEnabled(false);
   emit changed(true);
}


void KSocksConfig::libSelection()
{
   base->_c_remove->setEnabled(true);
}


void KSocksConfig::load()
{
  KConfigGroup config(kapp->config(), "Socks");
  base->_c_enableSocks->setChecked(config.readBoolEntry("SOCKS_enable", false));
  int id = config.readNumEntry("SOCKS_method", 1);
  base->bg->setButton(id);
  if (id == 4) {
    base->_c_customLabel->setEnabled(true);
    base->_c_customPath->setEnabled(true);
  } else {
    base->_c_customLabel->setEnabled(false);
    base->_c_customPath->setEnabled(false);
  }
  base->_c_customPath->setURL(config.readEntry("SOCKS_lib", ""));

  QListViewItem *thisitem;
  while ((thisitem = base->_c_libs->firstChild())) {
     base->_c_libs->takeItem(thisitem);
     delete thisitem;
  }

  QStringList libs = config.readListEntry("SOCKS_lib_path");
  for(QStringList::Iterator it = libs.begin();
                            it != libs.end();
                            ++it ) {
     new QListViewItem(base->_c_libs, *it);
  }
  base->_c_libs->clearSelection();
  base->_c_remove->setEnabled(false);
  base->_c_add->setEnabled(false);
  base->_c_newPath->clear();
  emit changed(false);
}

void KSocksConfig::save()
{
  KConfigGroup config(kapp->config(), "Socks");
  config.writeEntry("SOCKS_enable",base-> _c_enableSocks->isChecked(), true, true);
  config.writeEntry("SOCKS_method", base->bg->id(base->bg->selected()), true, true);
  config.writeEntry("SOCKS_lib", base->_c_customPath->url(), true, true);
  QListViewItem *thisitem = base->_c_libs->firstChild();

  QStringList libs;
  while (thisitem) {
    libs << thisitem->text(0);
    thisitem = thisitem->itemBelow();
  }
  config.writeEntry("SOCKS_lib_path", libs, ',', true, true);

  kapp->config()->sync();

  emit changed(false);
}

void KSocksConfig::defaults()
{

  base->_c_enableSocks->setChecked(false);
  base->bg->setButton(1);
  base->_c_customLabel->setEnabled(false);
  base->_c_customPath->setEnabled(false);
  base->_c_customPath->setURL("");
  QListViewItem *thisitem;
  while ((thisitem = base->_c_libs->firstChild())) {
     base->_c_libs->takeItem(thisitem);
     delete thisitem;
  }
  base->_c_newPath->clear();
  base->_c_add->setEnabled(false);
  base->_c_remove->setEnabled(false);
  emit changed(true);
}

QString KSocksConfig::quickHelp() const
{
  return i18n("<h1>SOCKS</h1><p>This module allows you to configure KDE support"
     " for a SOCKS server or proxy.</p><p>SOCKS is a protocol to traverse firewalls"
     " as described in <a href=\"http://rfc.net/rfc1928.html\">RFC 1928</a>."
     " <p>If you have no idea what this is and if your system administrator doesn't"
     " tell you to use it, leave it disabled.</p>");
}

const KAboutData* KSocksConfig::aboutData() const
{

    KAboutData *about =
    new KAboutData(I18N_NOOP("kcmsocks"), I18N_NOOP("KDE SOCKS Control Module"),
                  0, 0, KAboutData::License_GPL,
                  I18N_NOOP("(c) 2001 George Staikos"));

    about->addAuthor("George Staikos", 0, "staikos@kde.org");

    return about;
}


#include "socks.moc"

