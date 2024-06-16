/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 Alexander Kellett <lypanov@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kbookmarkimporter.h"

#include "kbookmarkimporter_ie.h"
#include "kbookmarkimporter_ns.h"
#include "kbookmarkimporter_opera.h"
#include "kbookmarkmanager.h"

#include <assert.h>
#include <stddef.h>

void KXBELBookmarkImporterImpl::parse()
{
    // qCDebug(KBOOKMARKS_LOG) << "KXBELBookmarkImporterImpl::parse()";
    KBookmarkManager manager(m_fileName);
    KBookmarkGroup root = manager.root();
    traverse(root);
}

void KXBELBookmarkImporterImpl::visit(const KBookmark &bk)
{
    // qCDebug(KBOOKMARKS_LOG) << "KXBELBookmarkImporterImpl::visit";
    if (bk.isSeparator()) {
        Q_EMIT newSeparator();
    } else {
        Q_EMIT newBookmark(bk.fullText(), bk.url().toString(), QLatin1String(""));
    }
}

void KXBELBookmarkImporterImpl::visitEnter(const KBookmarkGroup &grp)
{
    // qCDebug(KBOOKMARKS_LOG) << "KXBELBookmarkImporterImpl::visitEnter";
    Q_EMIT newFolder(grp.fullText(), false, QLatin1String(""));
}

void KXBELBookmarkImporterImpl::visitLeave(const KBookmarkGroup &)
{
    // qCDebug(KBOOKMARKS_LOG) << "KXBELBookmarkImporterImpl::visitLeave";
    Q_EMIT endFolder();
}

void BookmarkImporterBase::setupSignalForwards(QObject *src, QObject *dst)
{
    // clang-format off
    connect(src, SIGNAL(newBookmark(QString,QString,QString)), dst, SIGNAL(newBookmark(QString,QString,QString)));
    connect(src, SIGNAL(newFolder(QString,bool,QString)), dst, SIGNAL(newFolder(QString,bool,QString)));
    // clang-format on
    connect(src, SIGNAL(newSeparator()), dst, SIGNAL(newSeparator()));
    connect(src, SIGNAL(endFolder()), dst, SIGNAL(endFolder()));
}

BookmarkImporterBase *BookmarkImporterBase::factory(const QString &type)
{
    if (type == QLatin1String("netscape")) {
        return new NSBookmarkImporterImpl;
    } else if (type == QLatin1String("mozilla")) {
        return new KMozillaBookmarkImporterImpl;
    } else if (type == QLatin1String("xbel")) {
        return new KXBELBookmarkImporterImpl;
    } else if (type == QLatin1String("ie")) {
        return new IEBookmarkImporterImpl;
    } else if (type == QLatin1String("opera")) {
        return new OperaBookmarkImporterImpl;
    } else {
        return nullptr;
    }
}

#include "moc_kbookmarkimporter.cpp"
