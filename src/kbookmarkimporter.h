//  -*- c-basic-offset:4; indent-tabs-mode:nil -*-
/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef __kbookmarkimporter_h
#define __kbookmarkimporter_h

#include <QObject>

#include "kbookmark.h"

/**
 * A class for importing NS bookmarks
 * KEditBookmarks uses it to insert bookmarks into its DOM tree,
 * and KActionMenu uses it to create actions directly.
 */
class BookmarkImporterBase : public QObject
{
    Q_OBJECT
public:
    BookmarkImporterBase()
    {
    }
    ~BookmarkImporterBase() override
    {
    }

    void setFilename(const QString &filename)
    {
        m_fileName = filename;
    }

    virtual void parse() = 0;
    virtual QString findDefaultLocation(bool forSaving = false) const = 0;

    // TODO - make this static?
    void setupSignalForwards(QObject *src, QObject *dst);
    static BookmarkImporterBase *factory(const QString &type);

Q_SIGNALS:
    /**
     * Notify about a new bookmark
     * Use "html" for the icon
     */
    void newBookmark(const QString &text, const QString &url, const QString &additionalInfo);

    /**
     * Notify about a new folder
     * Use "bookmark_folder" for the icon
     */
    void newFolder(const QString &text, bool open, const QString &additionalInfo);

    /**
     * Notify about a new separator
     */
    void newSeparator();

    /**
     * Tell the outside world that we're going down
     * one menu
     */
    void endFolder();

protected:
    QString m_fileName;

private:
    class KBookmarkImporterBasePrivate *d;
};

/**
 * A class for importing XBEL files
 */
class KXBELBookmarkImporterImpl : public BookmarkImporterBase, protected KBookmarkGroupTraverser
{
    Q_OBJECT
public:
    KXBELBookmarkImporterImpl()
    {
    }
    void parse() override;
    QString findDefaultLocation(bool = false) const override
    {
        return QString();
    }

protected:
    void visit(const KBookmark &) override;
    void visitEnter(const KBookmarkGroup &) override;
    void visitLeave(const KBookmarkGroup &) override;

private:
    class KXBELBookmarkImporterImplPrivate *d;
};

#endif
