/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1996-1998 Martin R. Jones <mjones@kde.org>
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2003 Alexander Kellett <lypanov@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef __kbookmarkexporter_h
#define __kbookmarkexporter_h

#include <KBookmark>

class BookmarkExporterBase
{
public:
    BookmarkExporterBase(const QString &fileName)
        : m_fileName(fileName)
    {
    }
    virtual ~BookmarkExporterBase()
    {
    }
    virtual void write(const KBookmarkGroup &) = 0;

protected:
    QString m_fileName;

private:
    class KBookmarkExporterBasePrivate *d;
};

#endif
