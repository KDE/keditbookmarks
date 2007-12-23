/*  This file is part of the KDE project
    Copyright (C) 2000 Alexander Neundorf <neundorf@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KSHELLCMDEXECUTOR_H
#define KSHELLCMDEXECUTOR_H


#include <QTextEdit>

namespace KDESu {class PtyProcess; }
class QSocketNotifier;

class KShellCommandExecutor:public QTextEdit
{
   Q_OBJECT
   public:
      explicit KShellCommandExecutor(const QString& command, QWidget* parent=0);
      virtual ~KShellCommandExecutor();
      int exec();
   Q_SIGNALS:
      void finished();
   public Q_SLOTS:
      void slotFinished();
   protected:
	   KDESu::PtyProcess *m_shellProcess;
      QString m_command;
      QSocketNotifier *m_readNotifier;
      QSocketNotifier *m_writeNotifier;
   protected Q_SLOTS:
      void readDataFromShell();
      void writeDataToShell();
};

#endif // KSHELLCMDEXECUTOR_H
