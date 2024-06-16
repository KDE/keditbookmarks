/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef __toplevel_h
#define __toplevel_h

#include "bookmarklistview.h"
#include <KBookmark>
#include <KXMLGUIFactory>
#include <QMenu>
#include <kxmlguiwindow.h>

class ActionsImpl;
class CommandHistory;
class KToggleAction;
class BookmarkInfoWidget;
class BookmarkListView;

struct SelcAbilities {
    bool itemSelected : 1;
    bool group : 1;
    bool root : 1;
    bool separator : 1;
    bool urlIsEmpty : 1;
    bool multiSelect : 1;
    bool singleSelect : 1;
    bool notEmpty : 1;
    bool deleteEnabled : 1;
};

class KEBApp : public KXmlGuiWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.keditbookmarks")
public:
    static KEBApp *self()
    {
        return s_topLevel;
    }

    KEBApp(const QString &bookmarksFile, bool readonly, const QString &address, bool browser, const QString &caption, const QString &dbusObjectName);
    ~KEBApp() override;

    void reset(const QString &caption, const QString &bookmarksFileName);

    void updateActions();
    SelcAbilities getSelectionAbilities() const;
    void setActionsEnabled(SelcAbilities);

    QMenu *popupMenuFactory(const QString &type)
    {
        QWidget *menu = factory()->container(type, this);
        return dynamic_cast<QMenu *>(menu);
    }

    QString caption() const
    {
        return m_caption;
    }
    bool readonly() const
    {
        return m_readOnly;
    }
    bool browser() const
    {
        return m_browser;
    }

    BookmarkInfoWidget *bkInfo()
    {
        return m_bkinfo;
    }

    void expandAll();
    void collapseAll();

    enum Column { NameColumn = 0, UrlColumn = 1, CommentColumn = 2, StatusColumn = 3 };
    void startEdit(Column c);
    KBookmark firstSelected() const;
    QString insertAddress() const;
    KBookmark::List selectedBookmarks() const;
    KBookmark::List selectedBookmarksExpanded() const;
    KBookmark::List allBookmarks() const;

public Q_SLOTS:
    void notifyCommandExecuted();

    Q_SCRIPTABLE QString bookmarkFilename();

public Q_SLOTS:
    void slotConfigureToolbars();

private Q_SLOTS:
    void slotClipboardDataChanged();
    void slotNewToolbarConfig();
    void selectionChanged();
    void setCancelFavIconUpdatesEnabled(bool);
    void setCancelTestsEnabled(bool);
    void slotManagerError(const QString &errorMessage);

private:
    void selectedBookmarksExpandedHelper(const KBookmark &bk, KBookmark::List &bookmarks) const;
    BookmarkListView *mBookmarkListView;
    BookmarkFolderView *mBookmarkFolderView;

private:
    void resetActions();
    void createActions();

    static KEBApp *s_topLevel;

    ActionsImpl *m_actionsImpl;
    CommandHistory *m_cmdHistory;
    QString m_bookmarksFilename;
    QString m_caption;
    QString m_dbusObjectName;

    BookmarkInfoWidget *m_bkinfo;

    bool m_canPaste : 1;
    bool m_readOnly : 1;
    bool m_browser : 1;
};

#endif
