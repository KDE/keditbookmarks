/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2002 Alexander Kellett <lypanov@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef __kbookmarkimporter_opera_h
#define __kbookmarkimporter_opera_h

#include <kbookmarkexporter.h>
#include <kbookmarkimporter.h>

/**
 * A class for importing Opera bookmarks
 */
class OperaBookmarkImporterImpl : public BookmarkImporterBase
{
    Q_OBJECT // For QObject::tr
        public : OperaBookmarkImporterImpl()
    {
    }
    void parse() override;
    QString findDefaultLocation(bool forSaving = false) const override;

private:
    class KOperaBookmarkImporterImplPrivate *d;
};

class KOperaBookmarkExporterImpl : public BookmarkExporterBase
{
public:
    KOperaBookmarkExporterImpl(KBookmarkManager *mgr, const QString &filename)
        : BookmarkExporterBase(mgr, filename)
    {
        ;
    }
    ~KOperaBookmarkExporterImpl() override
    {
    }
    void write(const KBookmarkGroup &parent) override;

private:
    class KOperaBookmarkExporterImplPrivate *d;
};

#endif
