/***************************************************************************
 *   Copyright (C) 2007-2010 by Peter Penz <peter.penz19@gmail.com>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "terminalpanel.h"

#include <signal.h>

#include <KPluginLoader>
#include <KPluginFactory>
#include <KService>
#include <kde_terminal_interface_v2.h>
#include <KParts/Part>
#include <KShell>
#include <KIO/Job>
#include <KIO/JobUiDelegate>

#include <QBoxLayout>
#include <QDir>
#include <QShowEvent>

TerminalPanel::TerminalPanel(QWidget* parent) :
    Panel(parent),
    m_clearTerminal(true),
    m_mostLocalUrlJob(0),
    m_layout(0),
    m_terminal(0),
    m_terminalWidget(0),
    m_konsolePart(0),
    m_konsolePartCurrentDirectory(),
    m_sendCdToTerminalHistory()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setMargin(0);
}

TerminalPanel::~TerminalPanel()
{
}

void TerminalPanel::terminalExited()
{
    m_terminal = 0;
    emit hideTerminalPanel();
}

void TerminalPanel::dockVisibilityChanged()
{
    // Only react when the DockWidget itself (not some parent) is hidden. This way we don't
    // respond when e.g. Dolphin is minimized.
    if (parentWidget() && parentWidget()->isHidden() &&
        m_terminal && (m_terminal->foregroundProcessId() == -1)) {
        // Make sure that the following "cd /" command will not affect the view.
        disconnect(m_konsolePart, SIGNAL(currentDirectoryChanged(QString)),
                   this, SLOT(slotKonsolePartCurrentDirectoryChanged(QString)));

        // Make sure this terminal does not prevent unmounting any removable drives
        changeDir(KUrl::fromPath("/"));

        // Because we have disconnected from the part's currentDirectoryChanged()
        // signal, we have to update m_konsolePartCurrentDirectory manually. If this
        // was not done, showing the panel again might not set the part's working
        // directory correctly.
        m_konsolePartCurrentDirectory = '/';
    }
}

bool TerminalPanel::urlChanged()
{
    if (!url().isValid()) {
        return false;
    }

    const bool sendInput = m_terminal && (m_terminal->foregroundProcessId() == -1) && isVisible();
    if (sendInput) {
        changeDir(url());
    }

    return true;
}

void TerminalPanel::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        Panel::showEvent(event);
        return;
    }

    if (!m_terminal) {
        m_clearTerminal = true;
        KPluginFactory* factory = 0;
        KService::Ptr service = KService::serviceByDesktopName("konsolepart");
        if (service) {
            factory = KPluginLoader(service->library()).factory();
        }
        m_konsolePart = factory ? (factory->create<KParts::ReadOnlyPart>(this)) : 0;
        if (m_konsolePart) {
            connect(m_konsolePart, SIGNAL(destroyed(QObject*)), this, SLOT(terminalExited()));
            m_terminalWidget = m_konsolePart->widget();
            m_layout->addWidget(m_terminalWidget);
            m_terminal = qobject_cast<TerminalInterfaceV2 *>(m_konsolePart);
        }
    }
    if (m_terminal) {
        m_terminal->showShellInDir(url().toLocalFile());
        changeDir(url());
        m_terminalWidget->setFocus();
        connect(m_konsolePart, SIGNAL(currentDirectoryChanged(QString)),
                this, SLOT(slotKonsolePartCurrentDirectoryChanged(QString)));
    }

    Panel::showEvent(event);
}

void TerminalPanel::changeDir(const KUrl& url)
{
    delete m_mostLocalUrlJob;
    m_mostLocalUrlJob = 0;

    if (url.isLocalFile()) {
        sendCdToTerminal(url.toLocalFile());
    } else {
        m_mostLocalUrlJob = KIO::mostLocalUrl(url, KIO::HideProgressInfo);
        if (m_mostLocalUrlJob->ui()) {
            m_mostLocalUrlJob->ui()->setWindow(this);
        }
        connect(m_mostLocalUrlJob, SIGNAL(result(KJob*)), this, SLOT(slotMostLocalUrlResult(KJob*)));
    }
}

void TerminalPanel::sendCdToTerminal(const QString& dir)
{
    if (dir == m_konsolePartCurrentDirectory) {
        m_clearTerminal = false;
        return;
    }

    if (!m_clearTerminal) {
        // The TerminalV2 interface does not provide a way to delete the
        // current line before sending a new input. This is mandatory,
        // otherwise sending a 'cd x' to a existing 'rm -rf *' might
        // result in data loss. As workaround SIGINT is send.
        const int processId = m_terminal->terminalProcessId();
        if (processId > 0) {
            kill(processId, SIGINT);
        }
    }

    m_terminal->sendInput(" cd " + KShell::quoteArg(dir) + '\n');

    // We want to ignore the currentDirectoryChanged(QString) signal, which we will receive after
    // the directory change, because this directory change is not caused by a "cd" command that the
    // user entered in the panel. Therefore, we have to remember 'dir'. Note that it could also be
    // a symbolic link -> remember the 'canonical' path.
    m_sendCdToTerminalHistory.enqueue(QDir(dir).canonicalPath());

    if (m_clearTerminal) {
        m_terminal->sendInput(" clear\n");
        m_clearTerminal = false;
    }
}

void TerminalPanel::slotMostLocalUrlResult(KJob* job)
{
    KIO::StatJob* statJob = static_cast<KIO::StatJob *>(job);
    const KUrl url = statJob->mostLocalUrl();
    if (url.isLocalFile()) {
        sendCdToTerminal(url.toLocalFile());
    }

    m_mostLocalUrlJob = 0;
}

void TerminalPanel::slotKonsolePartCurrentDirectoryChanged(const QString& dir)
{
    m_konsolePartCurrentDirectory = QDir(dir).canonicalPath();

    // Only emit a changeUrl signal if the directory change was caused by the user inside the
    // terminal, and not by sendCdToTerminal(QString).
    while (!m_sendCdToTerminalHistory.empty()) {
        if (m_konsolePartCurrentDirectory == m_sendCdToTerminalHistory.dequeue()) {
            return;
        }
    }

    emit changeUrl(dir);
}

#include "terminalpanel.moc"
