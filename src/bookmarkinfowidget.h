/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef BOOKMARKINFOWIDGET_H
#define BOOKMARKINFOWIDGET_H

#include <KBookmark>
#include <QWidget>

class KBookmarkModel;
class BookmarkListView;
class EditCommand;

class KLineEdit;

class QTimer;

class BookmarkInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BookmarkInfoWidget(BookmarkListView *lv, KBookmarkModel *model, QWidget * = nullptr);

    KBookmark bookmark()
    {
        return m_bk;
    }

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
    void updateStatus();
    void showBookmark(const KBookmark &bk);

    EditCommand *titlecmd, *urlcmd, *commentcmd;
    QTimer *timer;
    KLineEdit *m_title_le, *m_url_le, *m_comment_le;
    KLineEdit *m_visitdate_le, *m_credate_le, *m_visitcount_le;
    KBookmark m_bk;
    KBookmarkModel *m_model;
    BookmarkListView *mBookmarkListView;
};

#endif
