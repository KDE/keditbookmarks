/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2006 David Faure <faure@kde.org>

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
#ifndef KONQ_FILEUNDOMANAGER_H
#define KONQ_FILEUNDOMANAGER_H

#include <QtCore/QObject>
#include <kurl.h>
#include <sys/types.h> // time_t

#include <libkonq_export.h>

namespace KIO
{
  class Job;
}
struct KonqBasicOperation;
class KonqCommand;
class KonqUndoJob;
class KJob;

/**
 * KonqFileUndoManager: makes it possible to undo kio jobs.
 * This class is a singleton, use self() to access its only instance.
 */
class LIBKONQ_EXPORT KonqFileUndoManager : public QObject
{
    Q_OBJECT
public:
    virtual ~KonqFileUndoManager();

    static void incRef();
    static void decRef();
    static KonqFileUndoManager *self();

    // Interface for the gui handling of KonqFileUndoManager
    class LIBKONQ_EXPORT UiInterface
    {
    public:
        UiInterface( QWidget* );
        virtual ~UiInterface() {}

        void setShowProgressInfo( bool b ) { m_showProgressInfo = b; }
        bool showProgressInfo() const { return m_showProgressInfo; }

        /**
         * Called when an undo job errors; default implementation displays a message box.
         */
        virtual void jobError( KIO::Job* job );

        /**
         * Called when we are about to remove those files.
         * Return true if we should proceed with deleting them.
         */
        virtual bool confirmDeletion( const KUrl::List& files );

        /**
         * Called when dest was modified since it was copied from src.
         * Note that this is called after confirmDeletion.
         * Return true if we should proceed with deleting dest.
         */
        virtual bool copiedFileWasModified( const KUrl& src, const KUrl& dest, time_t srcTime, time_t destTime );
    private:
        QWidget* m_parentWidget;
        bool m_showProgressInfo;
    };

    /**
     * Set a new UiInterface implementation.
     * This deletes the previous one.
     */
    void setUiInterface( UiInterface* ui );

    enum CommandType { COPY, MOVE, RENAME, LINK, MKDIR, TRASH };

    /**
     * Record this job while it's happening and add a command for it so that the user can undo it.
     * @param op the type of job - which is also the type of command that will be created for it
     * @param src list of source urls
     * @param dst destination url
     * @param job the job to record
     */
    void recordJob( CommandType op, const KUrl::List &src, const KUrl &dst, KIO::Job *job );

    bool undoAvailable() const;
    QString undoText() const;

    /**
     * These two functions are useful when wrapping KonqFileUndoManager and adding custom commands.
     * Each command has a unique ID. You can get a new serial number for a custom command
     * with newCommandSerialNumber(), and then when you want to undo, check if the command
     * KonqFileUndoManager would undo is newer or older than your custom command.
     */
    quint64 newCommandSerialNumber();
    quint64 currentCommandSerialNumber();

public Q_SLOTS:
    // TODO: add QWidget* parameter for error boxes
    /// Undoes the last command
    void undo();

    /// @internal called by KonqFileUndoManagerAdaptor
    QByteArray get() const;

Q_SIGNALS:
    /// Emitted when the value of undoAvailable() changes
    void undoAvailable( bool avail );

    /// Emitted when the value of undoText() changes
    void undoTextChanged( const QString &text );

    /// Emitted when an undo job finishes. Used for unit testing.
    void undoJobFinished();

    // The four signals below are emitted to DBus
    void push( const QByteArray &command );
    void pop();
    void lock();
    void unlock();

private Q_SLOTS:
    // Those four are connected to DBUS signals
    void slotPush( QByteArray command ); // const ref doesn't work due to QDataStream
    void slotPop();
    void slotLock();
    void slotUnlock();

private Q_SLOTS:
    void slotResult( KJob *job );

private:
    KonqFileUndoManager();

    friend class KonqUndoJob;
    /// called by KonqUndoJob
    void stopUndo( bool step );

    friend class KonqCommandRecorder;
    /// called by KonqCommandRecorder
    void addCommand( const KonqCommand &cmd );

    void pushCommand( const KonqCommand& cmd );
    void undoStep();

    void stepMakingDirectories();
    void stepMovingFiles();
    void stepRemovingLinks();
    void stepRemovingDirectories();

    void broadcastPush( const KonqCommand &cmd );
    void broadcastPop();
    void broadcastLock();
    void broadcastUnlock();

    void addDirToUpdate( const KUrl& url );
    bool initializeFromKDesky();

    class KonqFileUndoManagerPrivate;
    KonqFileUndoManagerPrivate *d;
    static KonqFileUndoManager *s_self;
};

#endif
