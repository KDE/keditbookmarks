/* This file is part of the KDE project
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "favicons.h"

#include "faviconupdater.h"
#include "kbookmarkmodel/model.h"

#include <KLocalizedString>

FavIconsItrHolder::FavIconsItrHolder(QObject *parent, KBookmarkModel *model)
    : BookmarkIteratorHolder(parent, model)
{
}

/* -------------------------- */

FavIconsItr::FavIconsItr(BookmarkIteratorHolder *holder, const QList<KBookmark> &bks)
    : BookmarkIterator(holder, bks)
    , m_updater(nullptr)
{
}

FavIconsItr::~FavIconsItr()
{
    delete m_updater;
}

void FavIconsItr::setStatus(const QString &status)
{
    currentBookmark().setMetaDataItem(QStringLiteral("favstate"), status);
    model()->emitDataChanged(currentBookmark());
}

void FavIconsItr::slotDone(bool succeeded, const QString &errorString)
{
    // //qCDebug(KEDITBOOKMARKS_LOG) << "FavIconsItr::slotDone()";
    setStatus(succeeded ? i18n("OK") : errorString);
    holder()->addAffectedBookmark(KBookmark::parentAddress(currentBookmark().address()));
    delayedEmitNextOne();
}

bool FavIconsItr::isApplicable(const KBookmark &bk) const
{
    if (bk.isGroup() || bk.isSeparator())
        return false;
    return bk.url().scheme().startsWith(QLatin1String("http"));
}

void FavIconsItr::doAction()
{
    // //qCDebug(KEDITBOOKMARKS_LOG) << "FavIconsItr::doAction()";
    m_oldStatus = currentBookmark().metaDataItem(QStringLiteral("favstate"));
    setStatus(i18n("Updating favicon..."));
    if (!m_updater) {
        m_updater = new FavIconUpdater(this);
        connect(m_updater, &FavIconUpdater::done, this, &FavIconsItr::slotDone);
    }
    m_updater->downloadIcon(currentBookmark());
}

void FavIconsItr::cancel()
{
    setStatus(m_oldStatus);
}

#include "moc_favicons.cpp"
