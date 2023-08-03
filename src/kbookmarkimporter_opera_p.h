/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2002 Alexander Kellett <lypanov@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KBOOKMARKIMPORTER_OPERA_P_H
#define KBOOKMARKIMPORTER_OPERA_P_H

/**
 * A class for importing Opera bookmarks
 * @internal
 */
class KOperaBookmarkImporter : public QObject
{
    Q_OBJECT
public:
    explicit KOperaBookmarkImporter(const QString &fileName)
        : m_fileName(fileName)
    {
    }
    ~KOperaBookmarkImporter() override
    {
    }

    void parseOperaBookmarks();

    // Usual place for Opera bookmarks
    static QString operaBookmarksFile();

Q_SIGNALS:
    void newBookmark(const QString &text, const QString &url, const QString &additionalInfo);
    void newFolder(const QString &text, bool open, const QString &additionalInfo);
    void newSeparator();
    void endFolder();

protected:
    QString m_fileName;
};

#endif /* KBOOKMARKIMPORTER_OPERA_P_H */
