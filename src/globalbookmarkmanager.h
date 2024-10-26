/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000, 2010 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef GLOBALBOOKMARKMANAGER_H
#define GLOBALBOOKMARKMANAGER_H

#include <KBookmark>
#include <QObject>

class CommandHistory;
class KBookmarkModel;
class KBookmarkManager;

class GlobalBookmarkManager : public QObject
{
    Q_OBJECT
public:
    typedef enum {
        HTMLExport,
        OperaExport,
        IEExport,
        MozillaExport,
        NetscapeExport
    } ExportType;

    // TODO port to K_GLOBAL_STATIC if we keep this class
    static GlobalBookmarkManager *self()
    {
        if (!s_mgr) {
            s_mgr = new GlobalBookmarkManager();
        }
        return s_mgr;
    }
    ~GlobalBookmarkManager() override;
    KBookmarkGroup root();
    static KBookmark bookmarkAt(const QString &a);

    KBookmarkModel *model() const
    {
        return m_model;
    }
    KBookmarkManager *mgr() const
    {
        return m_mgr.get();
    }
    QString path() const;

    void createManager(const QString &filename, const QString &dbusObjectName, CommandHistory *commandHistory);
    void notifyManagers(const KBookmarkGroup &grp);
    void notifyManagers();
    bool managerSave();
    void saveAs(const QString &fileName);
    void doExport(ExportType type, const QString &path = QString());

    // TODO move out
    static QString makeTimeStr(const QString &);
    static QString makeTimeStr(int);

Q_SIGNALS:
    void error(const QString &errorMessage);

private:
    GlobalBookmarkManager();
    std::unique_ptr<KBookmarkManager> m_mgr;
    KBookmarkModel *m_model;
    static GlobalBookmarkManager *s_mgr;
};

#endif /* GLOBALBOOKMARKMANAGER_H */
