/**
 *  Copyright (c) Martin R. Jones 1996
 *  Copyright (c) Bernd Wuebben 1998
 *  Copyright (c) Christian Tibirna 1998
 *  Copyright 1998-2007 David Faure <faure@kde.org>
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

//
//
// "Desktop Options" Tab for KDesktop configuration
//
// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
//
// Layouts
// (c) Christian Tibirna 1998
// Port to KControl, split from Misc Tab, Port to KControl2
// (c) David Faure 1998
// Desktop menus, paths
// (c) David Faure 2000


// Own
#include "globalpaths.h"

// Qt
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QDesktopWidget>
#include <QtGui/QApplication>
#include <QtDBus/QtDBus>

// KDE
#include <kconfiggroup.h>
#include <kcustommenueditor.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kglobalsettings.h>
#include <kio/copyjob.h>
#include <kio/deletejob.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>

#include "konqkcmfactory.h"

// Local
#include <config-apps.h>

//-----------------------------------------------------------------------------

DesktopPathConfig::DesktopPathConfig(QWidget *parent, const QVariantList &)
    : KCModule( KonqKcmFactory::componentData(), parent )
{
  QLabel * tmpLabel;

#undef RO_LASTROW
#undef RO_LASTCOL
#define RO_LASTROW 4   // 3 paths + last row
#define RO_LASTCOL 2

  int row = 0;
  QGridLayout *lay = new QGridLayout(this );
  lay->setSpacing( KDialog::spacingHint() );
  lay->setMargin(0);

  lay->setRowStretch(RO_LASTROW,10); // last line grows

  lay->setColumnStretch(0,0);
  lay->setColumnStretch(1,0);
  lay->setColumnStretch(2,10);


  setQuickHelp( i18n("<h1>Paths</h1>\n"
    "This module allows you to choose where in the filesystem the "
    "files on your desktop should be stored.\n"
    "Use the \"Whats This?\" (Shift+F1) to get help on specific options."));

  // Desktop Paths
  row++;
  tmpLabel = new QLabel(i18n("Des&ktop path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  urDesktop = new KUrlRequester(this);
  urDesktop->setMode( KFile::Directory | KFile::LocalOnly );
  tmpLabel->setBuddy( urDesktop );
  lay->addWidget(urDesktop, row, 1, 1, RO_LASTCOL);
  connect(urDesktop, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  QString wtstr = i18n("This folder contains all the files"
                       " which you see on your desktop. You can change the location of this"
                       " folder if you want to, and the contents will move automatically"
                       " to the new location as well.");
  tmpLabel->setWhatsThis( wtstr );
  urDesktop->setWhatsThis( wtstr );

  row++;
  tmpLabel = new QLabel(i18n("A&utostart path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  urAutostart = new KUrlRequester(this);
  urAutostart->setMode( KFile::Directory | KFile::LocalOnly );
  tmpLabel->setBuddy( urAutostart );
  lay->addWidget(urAutostart, row, 1, 1, RO_LASTCOL);
  connect(urAutostart, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  wtstr = i18n("This folder contains applications or"
               " links to applications (shortcuts) that you want to have started"
               " automatically whenever KDE starts. You can change the location of this"
               " folder if you want to, and the contents will move automatically"
               " to the new location as well.");
  tmpLabel->setWhatsThis( wtstr );
  urAutostart->setWhatsThis( wtstr );

  row++;
  tmpLabel = new QLabel(i18n("D&ocuments path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  urDocument = new KUrlRequester(this);
  urDocument->setMode( KFile::Directory | KFile::LocalOnly );
  tmpLabel->setBuddy( urDocument );
  lay->addWidget(urDocument, row, 1, 1, RO_LASTCOL);
  connect(urDocument, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  wtstr = i18n("This folder will be used by default to "
               "load or save documents from or to.");
  tmpLabel->setWhatsThis( wtstr );
  urDocument->setWhatsThis( wtstr );

  // -- Bottom --
  Q_ASSERT( row == RO_LASTROW-1 ); // if it fails here, check the row++ and RO_LASTROW above
}

void DesktopPathConfig::load()
{
    // Desktop Paths
    urDesktop->setPath( KGlobalSettings::desktopPath() );
    urAutostart->setPath( KGlobalSettings::autostartPath() );
    urDocument->setPath( KGlobalSettings::documentPath() );
    changed();
}

void DesktopPathConfig::defaults()
{
    // Desktop Paths - keep defaults in sync with kglobalsettings.cpp
    urDesktop->setPath( QDir::homePath() + "/Desktop/" );
    urAutostart->setPath( KGlobal::dirs()->localkdedir() + "Autostart/" );
    urDocument->setPath( QDir::homePath() );
}

void DesktopPathConfig::save()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup configGroup( config, "Paths" );

    bool pathChanged = false;
    bool autostartMoved = false;

    KUrl desktopURL;
    desktopURL.setPath( KGlobalSettings::desktopPath() );
    KUrl newDesktopURL = urDesktop->url();

    KUrl autostartURL;
    autostartURL.setPath( KGlobalSettings::autostartPath() );
    KUrl newAutostartURL = urAutostart->url();

    KUrl documentURL;
    documentURL.setPath( KGlobalSettings::documentPath() );
    KUrl newDocumentURL = urDocument->url();

    if ( !newDesktopURL.equals( desktopURL, KUrl::CompareWithoutTrailingSlash ) )
    {
        // Test which other paths were inside this one (as it is by default)
        // and for each, test where it should go.
        // * Inside destination -> let them be moved with the desktop (but adjust name if necessary)
        // * Not inside destination -> move first
        // !!!
        kDebug() << "desktopURL=" << desktopURL;
        QString urlDesktop = urDesktop->url().path();
        if ( !urlDesktop.endsWith( "/" ))	//krazy:exclude=doublequote_chars
            urlDesktop+='/';

        if ( desktopURL.isParentOf( autostartURL ) )
        {
            kDebug() << "Autostart is on the desktop";

            // Either the Autostart field wasn't changed (-> need to update it)
            if ( newAutostartURL.equals( autostartURL, KUrl::CompareWithoutTrailingSlash ) )
            {
                // Hack. It could be in a subdir inside desktop. Hmmm... Argl.
                urAutostart->setPath( urlDesktop + "Autostart/" );
                kDebug() << "Autostart is moved with the desktop";
                autostartMoved = true;
            }
            // or it has been changed (->need to move it from here)
            else
            {
                KUrl futureAutostartURL;
                futureAutostartURL.setPath( urlDesktop + "Autostart/" );
                if ( newAutostartURL.equals( futureAutostartURL, KUrl::CompareWithoutTrailingSlash ) )
                    autostartMoved = true;
                else
                    autostartMoved = moveDir( KUrl( KGlobalSettings::autostartPath() ), KUrl( urAutostart->url() ), i18n("Autostart") );
            }
        }

        if ( moveDir( KUrl( KGlobalSettings::desktopPath() ), KUrl( urlDesktop ), i18n("Desktop") ) )
        {
            configGroup.writePathEntry( "Desktop", urlDesktop, KConfigBase::Normal | KConfigBase::Global );
            pathChanged = true;
        }
    }

    if ( !newAutostartURL.equals( autostartURL, KUrl::CompareWithoutTrailingSlash ) )
    {
        if (!autostartMoved)
            autostartMoved = moveDir( KUrl( KGlobalSettings::autostartPath() ), KUrl( urAutostart->url() ), i18n("Autostart") );
        if (autostartMoved)
        {
//            configGroup.writeEntry( "Autostart", Autostart->url());
            configGroup.writePathEntry( "Autostart", urAutostart->url().path(), KConfigBase::Normal | KConfigBase::Global );
            pathChanged = true;
        }
    }

    if ( !newDocumentURL.equals( documentURL, KUrl::CompareWithoutTrailingSlash ) )
    {
        bool pathOk = true;
        QString path = urDocument->url().path();
        if (!QDir(path).exists())
        {
            if (!KStandardDirs::makeDir(path))
            {
                KMessageBox::sorry(this, KIO::buildErrorString(KIO::ERR_COULD_NOT_MKDIR, path));
                urDocument->setPath(documentURL.path());
                pathOk = false;
            }
        }

        if (pathOk)
        {
            configGroup.writePathEntry( "Documents", path, KConfigBase::Normal | KConfigBase::Global );
            pathChanged = true;
        }
    }

    config->sync();

    if (pathChanged)
    {
        kDebug() << "DesktopPathConfig::save sending message SettingsChanged";
        KGlobalSettings::self()->emitChange(KGlobalSettings::SettingsChanged, KGlobalSettings::SETTINGS_PATHS);
    }

    // TODO connect the apps to the settingsChanged() signal from KGlobalSettings instead
    // Tell kdesktop about the new config file
#if 0
#ifdef Q_WS_X11
    int konq_screen_number = KApplication::desktop()->primaryScreen();
    QByteArray appname;
    if (konq_screen_number == 0)
        appname = "org.kde.kdesktop";
    else
        appname = "org.kde.kdesktop-screen-" + QByteArray::number(konq_screen_number);
    org::kde::kdesktop::Desktop desktop(appname, "/Desktop", QDBusConnection::sessionBus());
    desktop.configure();
#endif
#endif
}

bool DesktopPathConfig::moveDir( const KUrl & src, const KUrl & dest, const QString & type )
{
    if (!src.isLocalFile() || !dest.isLocalFile())
        return true;
    m_ok = true;
    // Ask for confirmation before moving the files
    if ( KMessageBox::questionYesNo( this, i18n("The path for '%1' has been changed.\nDo you want the files to be moved from '%2' to '%3'?",
                              type, src.path(), dest.path()), i18n("Confirmation Required"),KGuiItem(i18nc("Move desktop files from old to new place", "Move")),KStandardGuiItem::cancel() )
            == KMessageBox::Yes )
    {
        bool destExists = QFile::exists(dest.path());
        if (destExists)
        {
            m_copyToDest = dest;
            m_copyFromSrc = src;
            KIO::ListJob* job = KIO::listDir( src );
            connect( job, SIGNAL( entries( KIO::Job *, const KIO::UDSEntryList& ) ),
                     this, SLOT( slotEntries( KIO::Job *, const KIO::UDSEntryList& ) ) );
            qApp->enter_loop();

            if (m_ok)
            {
                KIO::del( src );
            }
        }
        else
        {
            KIO::Job * job = KIO::move( src, dest );
            job->ui()->setWindow(this);
            connect( job, SIGNAL( result( KJob * ) ), this, SLOT( slotResult( KJob * ) ) );
            // wait for job
            qApp->enter_loop();
        }
    }
    kDebug() << "DesktopPathConfig::slotResult returning " << m_ok;
    return m_ok;
}

void DesktopPathConfig::slotEntries( KIO::Job * job, const KIO::UDSEntryList& list)
{
    if (job->error())
    {
        job->ui()->setWindow(this);
        job->ui()->showErrorMessage();
        return;
    }

    QListIterator<KIO::UDSEntry> it(list);
    while (it.hasNext())
    {
        KFileItem file(it.next(), m_copyFromSrc, true, true);
        if (file.url() == m_copyFromSrc || file.url().fileName() == "..")
        {
            continue;
        }

        KIO::Job * moveJob = KIO::move( file.url(), m_copyToDest );
        moveJob->ui()->setWindow(this);
        connect( moveJob, SIGNAL( result( KJob * ) ), this, SLOT( slotResult( KJob * ) ) );
        qApp->enter_loop();
    }
    qApp->exit_loop();
}

void DesktopPathConfig::slotResult( KJob * job )
{
    if (job->error())
    {
        if ( job->error() != KIO::ERR_DOES_NOT_EXIST )
            m_ok = false;
        // If the source doesn't exist, no wonder we couldn't move the dir.
        // In that case, trust the user and set the new setting in any case.

        static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
    }
    qApp->exit_loop();
}

#include "globalpaths.moc"
