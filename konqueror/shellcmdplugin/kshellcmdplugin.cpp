/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kshellcmdplugin.h"
#include "kshellcmddialog.h"
#include <kparts/part.h>
#include <kicon.h>
#include <kactioncollection.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kshell.h>
#include <kapplication.h>
#include <kgenericfactory.h>
#include <kauthorized.h>
#include <kio/netaccess.h>

KShellCmdPlugin::KShellCmdPlugin( QObject* parent, const QStringList & )
    : KParts::Plugin( parent )
{
    if (!KAuthorized::authorizeKAction("shell_access"))
       return;

    KAction *action = actionCollection()->addAction("executeshellcommand");
    action->setIcon(KIcon("system-run"));
    action->setText(i18n( "&Execute Shell Command..." ));
    connect(action, SIGNAL(triggered(bool)), SLOT( slotExecuteShellCommand() ));
    action->setShortcut(Qt::CTRL+Qt::Key_E);
}

void KShellCmdPlugin::slotExecuteShellCommand()
{
    KParts::ReadOnlyPart * part = dynamic_cast<KParts::ReadOnlyPart *>(parent());
    if ( !part )
    {
        KMessageBox::sorry(0L, i18n("KShellCmdPlugin::slotExecuteShellCommand: Program error, please report a bug."));
        return;
    }
    KUrl url = KIO::NetAccess::mostLocalUrl(part->url(),NULL);
    if ( !url.isLocalFile() )
    {
        KMessageBox::sorry(part->widget(),i18n("Executing shell commands works only on local directories."));
        return;
    }
    QString path;
#if 0 // to be ported if still needed
    if ( part->currentItem() )
    {
        // Putting the complete path to the selected file isn't really necessary,
        // since we'll cd to the directory first. But we do need to get the
        // complete relative path.
        path = KUrl::relativePath( url.path(),
                                   part->currentItem()->url().path() );
    }
    else
#endif
    {
        path = url.toLocalFile();
    }
   bool ok;
   QString cmd = KInputDialog::getText( i18n("Execute Shell Command"),
      i18n( "Execute shell command in current directory:" ),
      KShell::quoteArg( path ), &ok, part->widget() );
   if ( ok )
   {
      QString chDir;
      chDir="cd ";
      chDir+=KShell::quoteArg(part->url().path());
      chDir+="; ";
      chDir+=cmd;

      KShellCommandDialog *shellCmdDialog=new KShellCommandDialog(i18n("Output from command: \"%1\"", cmd),chDir,part->widget(),true);
      shellCmdDialog->resize(500,300);
      shellCmdDialog->executeCommand();
      delete shellCmdDialog;
   }
}

typedef KGenericFactory<KShellCmdPlugin> KonqShellCmdPluginFactory;
K_EXPORT_COMPONENT_FACTORY( konq_shellcmdplugin, KonqShellCmdPluginFactory( "kshellcmdplugin" ) )

#include "kshellcmdplugin.moc"

