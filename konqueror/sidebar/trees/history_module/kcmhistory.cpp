/*
 *  kcmhistory.cpp
 *  Copyright (c) 2002 Stephan Binner <binner@kde.org>
 *
 *  based on kcmtaskbar.cpp
 *  Copyright (c) 2000 Kurt Granroth <granroth@kde.org>
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
 */

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>

#include <dcopclient.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kfontdialog.h>
#include <kgenericfactory.h>
#include <kglobal.h>
#include <knuminput.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knuminput.h>

#include "konq_historymgr.h"

#include "kcmhistory.h"
#include "history_settings.h"

typedef KGenericFactory<HistorySidebarConfig, QWidget > SMSFactory;
K_EXPORT_COMPONENT_FACTORY (kcm_history, SMSFactory("kcmhistory") );

HistorySidebarConfig::HistorySidebarConfig( QWidget *parent, const char* name, const QStringList & )
  : KCModule (SMSFactory::instance(), parent, name)
{
    KGlobal::locale()->insertCatalogue("konqueror");

    m_settings = new KonqSidebarHistorySettings( 0, "history settings" );
    m_settings->readSettings( false );

    QVBoxLayout *topLayout = new QVBoxLayout(this, 0, KDialog::spacingHint());
    dialog = new KonqSidebarHistoryDlg(this);

    dialog->spinEntries->setRange( 1, INT_MAX, 1, false );
    dialog->spinExpire->setRange(  1, INT_MAX, 1, false );

    dialog->spinNewer->setRange( 0, INT_MAX, 1, false );
    dialog->spinOlder->setRange( 0, INT_MAX, 1, false );

    dialog->comboNewer->insertItem( i18n("Minutes"), KonqSidebarHistorySettings::MINUTES );
    dialog->comboNewer->insertItem( i18n("Days"), KonqSidebarHistorySettings::DAYS );

    dialog->comboOlder->insertItem( i18n("Minutes"), KonqSidebarHistorySettings::MINUTES );
    dialog->comboOlder->insertItem( i18n("Days"), KonqSidebarHistorySettings::DAYS );

    connect( dialog->cbExpire, SIGNAL( toggled( bool )),
	     dialog->spinExpire, SLOT( setEnabled( bool )));
    connect( dialog->spinExpire, SIGNAL( valueChanged( int )),
	     this, SLOT( slotExpireChanged( int )));

    connect( dialog->spinNewer, SIGNAL( valueChanged( int )),
	     SLOT( slotNewerChanged( int )));
    connect( dialog->spinOlder, SIGNAL( valueChanged( int )),
	     SLOT( slotOlderChanged( int )));

    connect( dialog->btnFontNewer, SIGNAL( clicked() ), SLOT( slotGetFontNewer() ));
    connect( dialog->btnFontOlder, SIGNAL( clicked() ), SLOT( slotGetFontOlder() ));
    connect( dialog->btnClearHistory, SIGNAL( clicked() ), SLOT( slotClearHistory() ));

    connect( dialog->cbDetailedTips, SIGNAL( toggled( bool )), SLOT( configChanged() ));
    connect( dialog->cbExpire, SIGNAL( toggled( bool )), SLOT( configChanged() ));
    connect( dialog->spinEntries, SIGNAL( valueChanged( int )), SLOT( configChanged() ));
    connect( dialog->comboNewer, SIGNAL( activated( int )), SLOT( configChanged() ));
    connect( dialog->comboOlder, SIGNAL( activated( int )), SLOT( configChanged() ));

    dialog->show();
    topLayout->add(dialog);
    load();

    KonqHistoryManager *mgr = new KonqHistoryManager( kapp, "history mgr" );
}

void HistorySidebarConfig::configChanged()
{
    emit changed(true);
}

void HistorySidebarConfig::load()
{
    KConfig *config = new KConfig("konquerorrc");
    config->setGroup("HistorySettings");
    dialog->spinExpire->setValue( config->readNumEntry( "Maximum age of History entries", 90) );
    dialog->spinEntries->setValue( config->readNumEntry( "Maximum of History entries", 500 ) );
    dialog->cbExpire->setChecked( dialog->spinExpire->value() > 0 );
    delete config;

    dialog->spinNewer->setValue( m_settings->m_valueYoungerThan );
    dialog->spinOlder->setValue( m_settings->m_valueOlderThan );

    dialog->comboNewer->setCurrentItem( m_settings->m_metricYoungerThan );
    dialog->comboOlder->setCurrentItem( m_settings->m_metricOlderThan );

    dialog->cbDetailedTips->setChecked( m_settings->m_detailedTips );

    m_fontNewer = m_settings->m_fontYoungerThan;
    m_fontOlder = m_settings->m_fontOlderThan;

    // enable/disable widgets
    dialog->spinExpire->setEnabled( dialog->cbExpire->isChecked() );

    slotExpireChanged( dialog->spinExpire->value() );
    slotNewerChanged( dialog->spinNewer->value() );
    slotOlderChanged( dialog->spinOlder->value() );

    emit changed(false);
}

void HistorySidebarConfig::save()
{
    Q_UINT32 age   = dialog->cbExpire->isChecked() ? dialog->spinExpire->value() : 0;
    Q_UINT32 count = dialog->spinEntries->value();

    KConfig *config = new KConfig("konquerorrc");
    config->setGroup("HistorySettings");
    config->writeEntry( "Maximum of History entries", count );
    config->writeEntry( "Maximum age of History entries", age );
    delete config;

    QByteArray dataAge;
    QDataStream streamAge( dataAge, IO_WriteOnly );
    streamAge << age << "foo";
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyMaxAge(Q_UINT32, QCString)", dataAge );

    QByteArray dataCount;
    QDataStream streamCount( dataCount, IO_WriteOnly );
    streamCount << count << "foo";
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyMaxAge(Q_UINT32, QCString)", dataCount );

    m_settings->m_valueYoungerThan = dialog->spinNewer->value();
    m_settings->m_valueOlderThan   = dialog->spinOlder->value();

    m_settings->m_metricYoungerThan = dialog->comboNewer->currentItem();
    m_settings->m_metricOlderThan   = dialog->comboOlder->currentItem();

    m_settings->m_detailedTips = dialog->cbDetailedTips->isChecked();

    m_settings->m_fontYoungerThan = m_fontNewer;
    m_settings->m_fontOlderThan   = m_fontOlder;

    m_settings->applySettings();

    emit changed(false);
}

void HistorySidebarConfig::defaults()
{
    dialog->spinEntries->setValue( 500 );
    dialog->cbExpire->setChecked( true );
    dialog->spinExpire->setValue( 90 );

    dialog->spinNewer->setValue( 1 );
    dialog->spinOlder->setValue( 2 );

    dialog->comboNewer->setCurrentItem( KonqSidebarHistorySettings::DAYS );
    dialog->comboOlder->setCurrentItem( KonqSidebarHistorySettings::DAYS );

    dialog->cbDetailedTips->setChecked( true );

    m_fontNewer = QFont();
    m_fontNewer.setItalic( true );
    m_fontOlder = QFont();

    emit changed(true);
}

QString HistorySidebarConfig::quickHelp() const
{
    return i18n("<h1>History Sidebar</h1>"
      " You can configure the history sidebar here.");
}

void HistorySidebarConfig::slotExpireChanged( int value )
{
    if ( value == 1 )
        dialog->spinExpire->setSuffix( i18n(" day") );
    else
        dialog->spinExpire->setSuffix( i18n(" days") );
    configChanged();
}

// change hour to days, minute to minutes and the other way round,
// depending on the value of the spinbox, and synchronize the two spinBoxes
// to enfore newer <= older.
void HistorySidebarConfig::slotNewerChanged( int value )
{
    const QString& days = i18n("Days");
    const QString& minutes = i18n("Minutes");

    if ( value == 1 ) {
	dialog->comboNewer->changeItem( i18n("Day"), KonqSidebarHistorySettings::DAYS );
	dialog->comboNewer->changeItem( i18n("Minute"), KonqSidebarHistorySettings::MINUTES );
    }
    else {
	dialog->comboNewer->changeItem( days, KonqSidebarHistorySettings::DAYS );
	dialog->comboNewer->changeItem( minutes, KonqSidebarHistorySettings::MINUTES);
    }

    if ( dialog->spinNewer->value() > dialog->spinOlder->value() )
	dialog->spinOlder->setValue( dialog->spinNewer->value() );
    configChanged();
}

void HistorySidebarConfig::slotOlderChanged( int value )
{
    const QString& days = i18n("Days");
    const QString& minutes = i18n("Minutes");

    if ( value == 1 ) {
	dialog->comboOlder->changeItem( i18n("Day"), KonqSidebarHistorySettings::DAYS );
	dialog->comboOlder->changeItem( i18n("Minute"), KonqSidebarHistorySettings::MINUTES );
    }
    else {
	dialog->comboOlder->changeItem( days, KonqSidebarHistorySettings::DAYS );
	dialog->comboOlder->changeItem( minutes, KonqSidebarHistorySettings::MINUTES);
    }

    if ( dialog->spinNewer->value() > dialog->spinOlder->value() )
	dialog->spinNewer->setValue( dialog->spinOlder->value() );
    configChanged();
}

void HistorySidebarConfig::slotGetFontNewer()
{
    (void) KFontDialog::getFont( m_fontNewer, false, this );
    configChanged();
}

void HistorySidebarConfig::slotGetFontOlder()
{
    (void) KFontDialog::getFont( m_fontOlder, false, this );
    configChanged();
}

void HistorySidebarConfig::slotClearHistory()
{
    if ( KMessageBox::questionYesNo( this,
				     i18n("Do you really want to clear "
					  "the entire history?"),
				     i18n("Clear History?") )
	 == KMessageBox::Yes )
	KonqHistoryManager::kself()->emitClear();
}

#include "kcmhistory.moc"
