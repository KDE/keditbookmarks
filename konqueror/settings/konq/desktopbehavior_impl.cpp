/**
 * (c) Martin R. Jones 1996
 * (c) Bernd Wuebben 1998
 * (c) Christian Tibirna 1998
 * (c) David Faure 1998, 2000
 * (c) John Firebaugh 2003
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

#include "desktopbehavior_impl.h"

#include <qlayout.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qwhatsthis.h>
#include <klistview.h>
#include <kservice.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kmimetype.h>
#include <ktrader.h>
#include <kapplication.h>
#include <kcustommenueditor.h>
#include <dcopclient.h>
#include <konq_defaults.h> // include default values directly from libkonq
#include <kipc.h>

DesktopBehaviorModule::DesktopBehaviorModule(KConfig *config, QWidget *parent, const char * )
    : KCModule( parent, "kcmkonq" )
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    m_behavior = new DesktopBehavior(config, this);
    layout->addWidget(m_behavior);
    connect(m_behavior, SIGNAL(changed()), this, SLOT(changed()));
}

void DesktopBehaviorModule::changed()
{
    setChanged( true );
}

class DesktopBehaviorPreviewItem : public QCheckListItem
{
public:
    DesktopBehaviorPreviewItem(DesktopBehavior *rootOpts, QListView *parent,
                const KService::Ptr &plugin, bool on)
        : QCheckListItem(parent, plugin->name(), CheckBox),
          m_rootOpts(rootOpts)
    {
        m_pluginName = plugin->desktopEntryName();
        setOn(on);
    }
    DesktopBehaviorPreviewItem(DesktopBehavior *rootOpts, QListView *parent,
                bool on)
        : QCheckListItem(parent, i18n("Sound Files"), CheckBox),
          m_rootOpts(rootOpts)
    {
        m_pluginName = "audio/";
        setOn(on);
    }
    const QString &pluginName() const { return m_pluginName; }

protected:
    virtual void stateChange( bool ) { m_rootOpts->changed(); }

private:
    DesktopBehavior *m_rootOpts;
    QString m_pluginName;
};


class DesktopBehaviorDevicesItem : public QCheckListItem
{
public:
    DesktopBehaviorDevicesItem(DesktopBehavior *rootOpts, QListView *parent,
                const QString name, const QString mimetype, bool on)
        : QCheckListItem(parent, name, CheckBox),
          m_rootOpts(rootOpts),m_mimeType(mimetype){setOn(on);}

    const QString &mimeType() const { return m_mimeType; }

protected:
    virtual void stateChange( bool ) { m_rootOpts->changed(); }

private:
    DesktopBehavior *m_rootOpts;
    QString m_mimeType;
};



static const char * s_choices[6] = { "", "WindowListMenu", "DesktopMenu", "AppMenu", "CustomMenu1", "CustomMenu2" };

DesktopBehavior::DesktopBehavior(KConfig *config, QWidget *parent, const char * )
    : DesktopBehaviorBase( parent, "kcmkonq" ), g_pConfig(config)
{
  QString strMouseButton1, strMouseButton3, strButtonTxt1, strButtonTxt3;

  /*
   * The text on this form depends on the mouse setting, which can be right
   * or left handed.  The outer button functionality is actually swapped
   *
   */
  bool leftHandedMouse = ( KGlobalSettings::mouseSettings().handed == KGlobalSettings::KMouseSettings::LeftHanded);

  connect(desktopMenuGroup, SIGNAL(clicked(int)), this, SIGNAL(changed()));
  connect(iconsEnabledBox, SIGNAL(clicked()), this, SLOT(enableChanged()));
  connect(showHiddenBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(vrootBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(autoLineupIconsBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(toolTipBox, SIGNAL(clicked()), this, SIGNAL(changed()));

  strMouseButton1 = i18n("&Left button:");
  strButtonTxt1 = i18n( "You can choose what happens when"
   " you click the left button of your pointing device on the desktop:");

  strMouseButton3 = i18n("Right b&utton:");
  strButtonTxt3 = i18n( "You can choose what happens when"
   " you click the right button of your pointing device on the desktop:");

  if ( leftHandedMouse )
  {
     qSwap(strMouseButton1, strMouseButton3);
     qSwap(strButtonTxt1, strButtonTxt3);
  }

  leftLabel->setText( strMouseButton1 );
  leftLabel->setBuddy( leftComboBox );
  fillMenuCombo( leftComboBox );
  connect(leftEditButton, SIGNAL(clicked()), this, SLOT(editButtonPressed()));
  connect(leftComboBox, SIGNAL(activated(int)), this, SIGNAL(changed()));
  connect(leftComboBox, SIGNAL(activated(int)), this, SLOT(comboBoxChanged()));
  QString wtstr = strButtonTxt1 +
                  i18n(" <ul><li><em>No action:</em> as you might guess, nothing happens!</li>"
                       " <li><em>Window list menu:</em> a menu showing all windows on all"
                       " virtual desktops pops up. You can click on the desktop name to switch"
                       " to that desktop, or on a window name to shift focus to that window,"
                       " switching desktops if necessary, and restoring the window if it is"
                       " hidden. Hidden or minimized windows are represented with their names"
                       " in parentheses.</li>"
                       " <li><em>Desktop menu:</em> a context menu for the desktop pops up."
                       " Among other things, this menu has options for configuring the display,"
                       " locking the screen, and logging out of KDE.</li>"
                       " <li><em>Application menu:</em> the \"K\" menu pops up. This might be"
                       " useful for quickly accessing applications if you like to keep the"
                       " panel (also known as \"Kicker\") hidden from view.</li></ul>");
  QWhatsThis::add( leftLabel, wtstr );
  QWhatsThis::add( leftComboBox, wtstr );

  middleLabel->setBuddy( middleComboBox );
  fillMenuCombo( middleComboBox );
  connect(middleEditButton, SIGNAL(clicked()), this, SLOT(editButtonPressed()));
  connect(middleComboBox, SIGNAL(activated(int)), this, SIGNAL(changed()));
  connect(middleComboBox, SIGNAL(activated(int)), this, SLOT(comboBoxChanged()));
  wtstr = i18n("You can choose what happens when"
               " you click the middle button of your pointing device on the desktop:"
               " <ul><li><em>No action:</em> as you might guess, nothing happens!</li>"
               " <li><em>Window list menu:</em> a menu showing all windows on all"
               " virtual desktops pops up. You can click on the desktop name to switch"
               " to that desktop, or on a window name to shift focus to that window,"
               " switching desktops if necessary, and restoring the window if it is"
               " hidden. Hidden or minimized windows are represented with their names"
               " in parentheses.</li>"
               " <li><em>Desktop menu:</em> a context menu for the desktop pops up."
               " Among other things, this menu has options for configuring the display,"
               " locking the screen, and logging out of KDE.</li>"
               " <li><em>Application menu:</em> the \"K\" menu pops up. This might be"
               " useful for quickly accessing applications if you like to keep the"
               " panel (also known as \"Kicker\") hidden from view.</li></ul>");
  QWhatsThis::add( middleLabel, wtstr );
  QWhatsThis::add( middleComboBox, wtstr );

  rightLabel->setText( strMouseButton3 );
  rightLabel->setBuddy( rightComboBox );
  fillMenuCombo( rightComboBox );
  connect(rightEditButton, SIGNAL(clicked()), this, SLOT(editButtonPressed()));
  connect(rightComboBox, SIGNAL(activated(int)), this, SIGNAL(changed()));
  connect(rightComboBox, SIGNAL(activated(int)), this, SLOT(comboBoxChanged()));
  wtstr = strButtonTxt3 +
          i18n(" <ul><li><em>No action:</em> as you might guess, nothing happens!</li>"
               " <li><em>Window list menu:</em> a menu showing all windows on all"
               " virtual desktops pops up. You can click on the desktop name to switch"
               " to that desktop, or on a window name to shift focus to that window,"
               " switching desktops if necessary, and restoring the window if it is"
               " hidden. Hidden or minimized windows are represented with their names"
               " in parentheses.</li>"
               " <li><em>Desktop menu:</em> a context menu for the desktop pops up."
               " Among other things, this menu has options for configuring the display,"
               " locking the screen, and logging out of KDE.</li>"
               " <li><em>Application menu:</em> the \"K\" menu pops up. This might be"
               " useful for quickly accessing applications if you like to keep the"
               " panel (also known as \"Kicker\") hidden from view.</li></ul>");
  QWhatsThis::add( rightLabel, wtstr );
  QWhatsThis::add( rightComboBox, wtstr );

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
  connect(enableDevicesBox, SIGNAL(clicked()), this, SLOT(enableChanged()));
#else
  enableDevicesBox->hide();
  devicesListView->hide();
#endif

  load();
}

void DesktopBehavior::fillDevicesListView()
{
    devicesListView->clear();
    devicesListView->setRootIsDecorated(false);
    KMimeType::List mimetypes = KMimeType::allMimeTypes();
    QValueListIterator<KMimeType::Ptr> it2(mimetypes.begin());
    g_pConfig->setGroup( "Devices" );
    enableDevicesBox->setChecked(g_pConfig->readBoolEntry("enabled",false));
    QString excludedDevices=g_pConfig->readEntry("exclude","kdedevice/hdd_mounted,kdedevice/hdd_unmounted,kdedevice/floppy_unmounted,kdedevice/cdrom_unmounted,kdedevice/floppy5_unmounted");
    for (; it2 != mimetypes.end(); ++it2) {
       if ( ((*it2)->name().startsWith("kdedevice/")) ||
          ((*it2)->name()=="print/printer") )
	{
    	    bool ok=excludedDevices.contains((*it2)->name())==0;
		new DesktopBehaviorDevicesItem (this, devicesListView, (*it2)->comment(), (*it2)->name(),ok);

        }
    }
}

void DesktopBehavior::saveDevicesListView()
{
#if defined(Q_OS_LINUX) || defined (Q_OS_FREEBSD)
    g_pConfig->setGroup( "Devices" );
    g_pConfig->writeEntry("enabled",enableDevicesBox->isChecked());
    QStringList exclude;
    for (DesktopBehaviorDevicesItem *it=static_cast<DesktopBehaviorDevicesItem *>(devicesListView->firstChild());
     	it; it=static_cast<DesktopBehaviorDevicesItem *>(it->nextSibling()))
    	{
		if (!it->isOn()) exclude << it->mimeType();
	    }
     g_pConfig->writeEntry("exclude",exclude);
#endif
}


void DesktopBehavior::fillMenuCombo( QComboBox * combo )
{
  combo->insertItem( i18n("No Action") );
  combo->insertItem( i18n("Window List Menu") );
  combo->insertItem( i18n("Desktop Menu") );
  combo->insertItem( i18n("Application Menu") );
  combo->insertItem( i18n("Custom Menu 1") );
  combo->insertItem( i18n("Custom Menu 2") );
}

void DesktopBehavior::load()
{
    g_pConfig->setGroup( "Desktop Icons" );
    bool bShowHidden = g_pConfig->readBoolEntry("ShowHidden", DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    showHiddenBox->setChecked(bShowHidden);
    //bool bVertAlign = g_pConfig->readBoolEntry("VertAlign", DEFAULT_VERT_ALIGN);
    KTrader::OfferList plugins = KTrader::self()->query("ThumbCreator");
    previewListView->clear();
    QStringList previews = g_pConfig->readListEntry("Preview");
    for (KTrader::OfferList::ConstIterator it = plugins.begin(); it != plugins.end(); ++it)
        new DesktopBehaviorPreviewItem(this, previewListView, *it, previews.contains((*it)->desktopEntryName()));
    new DesktopBehaviorPreviewItem(this, previewListView, previews.contains("audio/"));
    //
    g_pConfig->setGroup( "FMSettings" );
    toolTipBox->setChecked(g_pConfig->readBoolEntry( "ShowFileTips", true ) );
    g_pConfig->setGroup( "Menubar" );
    KConfig config( "kdeglobals" );
    config.setGroup("KDE");
    bool globalMenuBar = config.readBoolEntry("macStyle", false);
    bool desktopMenuBar = g_pConfig->readBoolEntry("ShowMenubar", false);
    if ( globalMenuBar )
        desktopMenuGroup->setButton( 2 );
    else if ( desktopMenuBar )
        desktopMenuGroup->setButton( 1 );
    else
        desktopMenuGroup->setButton( 0 );
    g_pConfig->setGroup( "General" );
    vrootBox->setChecked( g_pConfig->readBoolEntry( "SetVRoot", false ) );
    iconsEnabledBox->setChecked( g_pConfig->readBoolEntry( "Enabled", true ) );
    autoLineupIconsBox->setChecked( g_pConfig->readBoolEntry( "AutoLineUpIcons", false ) );

    //
    g_pConfig->setGroup( "Mouse Buttons" );
    QString s;
    s = g_pConfig->readEntry( "Left", "" );
    for ( int c = 0 ; c < 6 ; c ++ )
    if (s == s_choices[c])
      { leftComboBox->setCurrentItem( c ); break; }
    s = g_pConfig->readEntry( "Middle", "WindowListMenu" );
    for ( int c = 0 ; c < 6 ; c ++ )
      if (s == s_choices[c])
      { middleComboBox->setCurrentItem( c ); break; }
    s = g_pConfig->readEntry( "Right", "DesktopMenu" );
    for ( int c = 0 ; c < 6 ; c ++ )
      if (s == s_choices[c])
      { rightComboBox->setCurrentItem( c ); break; }

    m_wheelSwitchesWorkspace = g_pConfig->readBoolEntry("WheelSwitchesWorkspace", false);

    comboBoxChanged();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    fillDevicesListView();
#endif
    enableChanged();
}

void DesktopBehavior::defaults()
{
    showHiddenBox->setChecked(DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    for (QListViewItem *item = previewListView->firstChild(); item; item = item->nextSibling())
        static_cast<DesktopBehaviorPreviewItem *>(item)->setOn(false);
    desktopMenuGroup->setButton( 0 );
    vrootBox->setChecked( false );
    autoLineupIconsBox->setChecked( true );
    leftComboBox->setCurrentItem( NOTHING );
    middleComboBox->setCurrentItem( WINDOWLISTMENU );
    rightComboBox->setCurrentItem( DESKTOPMENU );
    iconsEnabledBox->setChecked(true);
    autoLineupIconsBox->setChecked(false);
    toolTipBox->setChecked(true);
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    fillDevicesListView();
#endif

    comboBoxChanged();
    enableChanged();
}


void DesktopBehavior::save()
{
    g_pConfig->setGroup( "Desktop Icons" );
    g_pConfig->writeEntry("ShowHidden", showHiddenBox->isChecked());
    QStringList previews;
    for ( DesktopBehaviorPreviewItem *item = static_cast<DesktopBehaviorPreviewItem *>( previewListView->firstChild() );
          item;
          item = static_cast<DesktopBehaviorPreviewItem *>( item->nextSibling() ) )
        if ( item->isOn() )
            previews.append( item->pluginName() );
    g_pConfig->writeEntry( "Preview", previews );
    g_pConfig->setGroup( "FMSettings" );
    g_pConfig->writeEntry( "ShowFileTips", toolTipBox->isChecked() );
    g_pConfig->setGroup( "Menubar" );
#if QT_VERSION >= 0x030200
    g_pConfig->writeEntry("ShowMenubar", desktopMenuGroup->selectedId() > 0);
#else
    g_pConfig->writeEntry("ShowMenubar", desktopMenuGroup->id(desktopMenuGroup->selected()) > 0);
#endif
    KConfig config( "kdeglobals" );
    config.setGroup("KDE");
#if QT_VERSION >= 0x030200
    bool globalMenuBar = desktopMenuGroup->selectedId() == 2;
#else
    bool globalMenuBar = desktopMenuGroup->id(desktopMenuGroup->selected()) == 2;
#endif
    if ( globalMenuBar != config.readBoolEntry("macStyle", false) )
    {
        config.writeEntry( "macStyle", globalMenuBar, true, true );
        config.sync();
        KIPC::sendMessageAll(KIPC::ToolbarStyleChanged);
    }
    g_pConfig->setGroup( "Mouse Buttons" );
    g_pConfig->writeEntry("Left", s_choices[ leftComboBox->currentItem() ] );
    g_pConfig->writeEntry("Middle", s_choices[ middleComboBox->currentItem() ]);
    g_pConfig->writeEntry("Right", s_choices[ rightComboBox->currentItem() ]);
    g_pConfig->writeEntry("WheelSwitchesWorkspace", m_wheelSwitchesWorkspace);

    g_pConfig->setGroup( "General" );
    g_pConfig->writeEntry( "SetVRoot", vrootBox->isChecked() );
    g_pConfig->writeEntry( "Enabled", iconsEnabledBox->isChecked() );
    g_pConfig->writeEntry( "AutoLineUpIcons", autoLineupIconsBox->isChecked() );

    saveDevicesListView();
    g_pConfig->sync();

    // Tell kdesktop about the new config file
    if ( !kapp->dcopClient()->isAttached() )
       kapp->dcopClient()->attach();
    QByteArray data;

    int konq_screen_number = KApplication::desktop()->primaryScreen();
    QCString appname;
    if (konq_screen_number == 0)
        appname = "kdesktop";
    else
        appname.sprintf("kdesktop-screen-%d", konq_screen_number);
    kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );
    kapp->dcopClient()->send( "menuapplet", "menuapplet", "configure()", data );
}

void DesktopBehavior::enableChanged()
{
    bool enabled = iconsEnabledBox->isChecked();
    showHiddenBox->setEnabled(enabled);
    previewListView->setEnabled(enabled);
    vrootBox->setEnabled(enabled);
    autoLineupIconsBox->setEnabled(enabled);

#if defined(Q_OS_LINUX) || defined (Q_OS_FREEBSD)
    enableDevicesBox->setEnabled(enabled);
    devicesListView->setEnabled(enableDevicesBox->isChecked() && iconsEnabledBox->isChecked());
#endif

    changed();
}

void DesktopBehavior::comboBoxChanged()
{
  // 4 - CustomMenu1
  // 5 - CustomMenu2
  int i;
  i = leftComboBox->currentItem();
  leftEditButton->setEnabled((i == 4) || (i == 5));
  i = middleComboBox->currentItem();
  middleEditButton->setEnabled((i == 4) || (i == 5));
  i = rightComboBox->currentItem();
  rightEditButton->setEnabled((i == 4) || (i == 5));
}

void DesktopBehavior::editButtonPressed()
{
   int i = 0;
   if (sender() == leftEditButton)
      i = leftComboBox->currentItem();
   if (sender() == middleEditButton)
      i = middleComboBox->currentItem();
   if (sender() == rightEditButton)
      i = rightComboBox->currentItem();

   QString cfgFile;
   if (i == 4) // CustomMenu1
      cfgFile = "kdesktop_custom_menu1";
   if (i == 5) // CustomMenu2
      cfgFile = "kdesktop_custom_menu2";

   if (cfgFile.isEmpty())
      return;

   KCustomMenuEditor editor(this);
   KConfig cfg(cfgFile, false, false);

   editor.load(&cfg);
   if (editor.exec())
   {
      editor.save(&cfg);
      cfg.sync();
      emit changed();
   }
}

QString DesktopBehavior::quickHelp() const
{
  return i18n("<h1>Behavior</h1>\n"
    "This module allows you to choose various options\n"
    "for your desktop, including the way in which icons are arranged and\n"
    "the pop-up menus associated with clicks of the middle and right mouse\n"
    "buttons on the desktop.\n"
    "Use the \"Whats This?\" (Shift+F1) to get help on specific options.");
}

#include "desktopbehavior_impl.moc"
