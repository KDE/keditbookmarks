/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kbookmarkdombuilder.h"
#include <kbookmarkmanager.h>

#include <QDebug>

BookmarkDomBuilder::BookmarkDomBuilder(const KBookmarkGroup &bkGroup, KBookmarkManager *manager)
{
    m_manager = manager;
    m_stack.push(bkGroup);
}

BookmarkDomBuilder::~BookmarkDomBuilder()
{
    m_list.clear();
    m_stack.clear();
}

void BookmarkDomBuilder::connectImporter(const QObject *importer)
{
    // clang-format off
    connect(importer, SIGNAL(newBookmark(QString,QString,QString)), SLOT(newBookmark(QString,QString,QString)));
    connect(importer, SIGNAL(newFolder(QString,bool,QString)), SLOT(newFolder(QString,bool,QString)));
    // clang-format on
    connect(importer, SIGNAL(newSeparator()), SLOT(newSeparator()));
    connect(importer, SIGNAL(endFolder()), SLOT(endFolder()));
}

void BookmarkDomBuilder::newBookmark(const QString &text, const QString &url, const QString &additionalInfo)
{
    if (!m_stack.isEmpty()) {
        KBookmark bk = m_stack.top().addBookmark(text, QUrl(url), QString());
        // store additional info
        bk.internalElement().setAttribute(QStringLiteral("netscapeinfo"), additionalInfo);
    } else {
        qWarning() << "m_stack is empty. This should not happen when importing a valid bookmarks file!";
    }
}

void BookmarkDomBuilder::newFolder(const QString &text, bool open, const QString &additionalInfo)
{
    if (!m_stack.isEmpty()) {
        // we use a qvaluelist so that we keep pointers to valid objects in the stack
        KBookmarkGroup gp = m_stack.top().createNewFolder(text);
        m_list.append(gp);
        m_stack.push(m_list.last());
        // store additional info
        QDomElement element = m_list.last().internalElement();
        element.setAttribute(QStringLiteral("netscapeinfo"), additionalInfo);
        element.setAttribute(QStringLiteral("folded"), open ? QStringLiteral("no") : QStringLiteral("yes"));
    } else {
        qWarning() << "m_stack is empty. This should not happen when importing a valid bookmarks file!";
    }
}

void BookmarkDomBuilder::newSeparator()
{
    if (!m_stack.isEmpty()) {
        m_stack.top().createNewSeparator();
    } else {
        qWarning() << "m_stack is empty. This should not happen when importing a valid bookmarks file!";
    }
}

void BookmarkDomBuilder::endFolder()
{
    if (!m_stack.isEmpty()) {
        m_stack.pop();
    } else {
        qWarning() << "m_stack is empty. This should not happen when importing a valid bookmarks file!";
    }
}

#include "moc_kbookmarkdombuilder.cpp"
