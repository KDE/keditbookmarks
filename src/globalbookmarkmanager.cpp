/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000, 2010 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#include "globalbookmarkmanager.h"
#include "kbookmarkmodel/commandhistory.h"
#include "kbookmarkmodel/model.h"
#include <KBookmarkManager>
#include <QDateTime>
#include <QLocale>

GlobalBookmarkManager *GlobalBookmarkManager::s_mgr = nullptr;

GlobalBookmarkManager::GlobalBookmarkManager()
    : QObject(nullptr)
    , m_mgr(nullptr)
    , m_model(nullptr)
{
}

GlobalBookmarkManager::~GlobalBookmarkManager()
{
}

KBookmarkGroup GlobalBookmarkManager::root()
{
    return mgr()->root();
}

KBookmark GlobalBookmarkManager::bookmarkAt(const QString &a)
{
    return self()->mgr()->findByAddress(a);
}

bool GlobalBookmarkManager::managerSave()
{
    return mgr()->save();
}

void GlobalBookmarkManager::saveAs(const QString &fileName)
{
    mgr()->saveAs(fileName);
}

QString GlobalBookmarkManager::path() const
{
    return mgr()->path();
}

void GlobalBookmarkManager::createManager(const QString &filename, const QString &dbusObjectName, CommandHistory *commandHistory)
{
    // qCDebug(KEDITBOOKMARKS_LOG)<<"DBus Object name: "<<dbusObjectName;
    Q_UNUSED(dbusObjectName);
    m_mgr = std::make_unique<KBookmarkManager>(filename);

    connect(m_mgr.get(), &KBookmarkManager::error, this, &GlobalBookmarkManager::error);

    commandHistory->setBookmarkManager(m_mgr.get());

    if (m_model) {
        m_model->setRoot(root());
    } else {
        m_model = new KBookmarkModel(root(), commandHistory, this);
    }
}

void GlobalBookmarkManager::notifyManagers(const KBookmarkGroup &grp)
{
    m_model->notifyManagers(grp);
}

void GlobalBookmarkManager::notifyManagers()
{
    notifyManagers(root());
}

QString GlobalBookmarkManager::makeTimeStr(const QString &in)
{
    int secs;
    bool ok;
    secs = in.toInt(&ok);
    if (!ok)
        return QString();
    return makeTimeStr(secs);
}

QString GlobalBookmarkManager::makeTimeStr(int b)
{
    QDateTime dt;
    dt.fromSecsSinceEpoch(b);
    QLocale l;
    return (dt.daysTo(QDateTime::currentDateTime()) > 31) ? l.toString(dt.date(), QLocale::LongFormat) : l.toString(dt, QLocale::LongFormat);
}

#include "moc_globalbookmarkmanager.cpp"
