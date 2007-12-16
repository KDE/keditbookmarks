// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef __bookmarkinfo_h
#define __bookmarkinfo_h

#include <kbookmark.h>
#include <QtGui/QWidget>

class BookmarkListView;
class EditCommand;

class KLineEdit;

class QTimer;

class BookmarkInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit BookmarkInfoWidget(BookmarkListView * lv, QWidget * = 0);
    
    KBookmark bookmark() { return m_bk; }
    void updateStatus(); //FIXME where was this called?

public Q_SLOTS:
    void slotTextChangedURL(const QString &);
    void slotTextChangedTitle(const QString &);
    void slotTextChangedComment(const QString &);

    void slotUpdate();

    void commitChanges();
    void commitTitle();
    void commitURL();
    void commitComment();

private:
    void showBookmark(const KBookmark &bk);
    EditCommand * titlecmd, * urlcmd, * commentcmd;
    QTimer * timer;
    KLineEdit *m_title_le, *m_url_le,
        *m_comment_le;
    KLineEdit  *m_visitdate_le, *m_credate_le,
              *m_visitcount_le;
    KBookmark m_bk;
    BookmarkListView * mBookmarkListView;
};

#endif
