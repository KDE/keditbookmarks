/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef __actionsimpl_h
#define __actionsimpl_h

#include <QObject>
class FavIconsItrHolder;
class TestLinkItrHolder;
class CommandHistory;
class KBookmarkModel;

class ActionsImpl : public QObject
{
    Q_OBJECT

public:
    ActionsImpl(QObject *parent, KBookmarkModel *model);
    ~ActionsImpl() override;
    bool save();

    TestLinkItrHolder *testLinkHolder()
    {
        return m_testLinkHolder;
    }
    FavIconsItrHolder *favIconHolder()
    {
        return m_favIconHolder;
    }

public Q_SLOTS:
    void slotLoad();
    void slotSaveAs();
    void slotCut();
    void slotCopy();
    void slotPaste();
    void slotRename();
    void slotChangeURL();
    void slotChangeComment();
    void slotChangeIcon();
    void slotDelete();
    void slotNewFolder();
    void slotNewBookmark();
    void slotInsertSeparator();
    void slotSort();
    void slotSetAsToolbar();
    void slotOpenLink();
    void slotTestSelection();
    void slotTestAll();
    void slotCancelAllTests();
    void slotUpdateFavIcon();
    void slotRecursiveSort();
    void slotUpdateAllFavIcons();
    void slotCancelFavIconUpdates();
    void slotExpandAll();
    void slotCollapseAll();
    void slotImport();
    void slotExportOpera();
    void slotExportHTML();
    void slotExportIE();
    void slotExportNS();
    void slotExportMoz();

private:
    CommandHistory *commandHistory();
    KBookmarkModel *m_model;
    TestLinkItrHolder *m_testLinkHolder;
    FavIconsItrHolder *m_favIconHolder;
};

#endif
