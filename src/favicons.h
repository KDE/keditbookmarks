/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef __favicons_h
#define __favicons_h

#include <KBookmark>

#include "bookmarkiterator.h"

class FavIconsItrHolder : public BookmarkIteratorHolder
{
public:
    FavIconsItrHolder(QObject *parent, KBookmarkModel *model);
};

class KBookmarkModel;
class FavIconUpdater;

class FavIconsItr : public BookmarkIterator
{
    Q_OBJECT

public:
    FavIconsItr(BookmarkIteratorHolder *holder, const QList<KBookmark> &bks);
    ~FavIconsItr() override;

    void cancel() override;

public Q_SLOTS:
    void slotDone(bool succeeded, const QString &errorString);

protected:
    void doAction() override;
    bool isApplicable(const KBookmark &bk) const override;

private:
    void setStatus(const QString &status);
    FavIconUpdater *m_updater;
    QString m_oldStatus;
};

#endif
