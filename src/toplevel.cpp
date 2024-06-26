/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#include "toplevel.h"
#include "globalbookmarkmanager.h"
#include <QVBoxLayout>

#include "kbookmarkmodel/model.h"

#include "actionsimpl.h"
#include "bookmarkinfowidget.h"
#include "kbookmarkmodel/commandhistory.h"
#include "kebsearchline.h"

#include <stdlib.h>

#include <QApplication>
#include <QClipboard>
#include <QSplitter>

#include <KActionCollection>
#include <KConfigGroup>
#include <KEditToolBar>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>

#include <QDBusConnection>
KEBApp *KEBApp::s_topLevel = nullptr;

KEBApp::KEBApp(const QString &bookmarksFile, bool readonly, const QString &address, bool browser, const QString &caption, const QString &dbusObjectName)
    : KXmlGuiWindow()
    , m_bookmarksFilename(bookmarksFile)
    , m_caption(caption)
    , m_dbusObjectName(dbusObjectName)
    , m_readOnly(readonly)
    , m_browser(browser)
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/keditbookmarks"), this, QDBusConnection::ExportScriptableSlots);
    Q_UNUSED(address); // FIXME sets the current item

    m_cmdHistory = new CommandHistory(this);
    m_cmdHistory->createActions(actionCollection());
    connect(m_cmdHistory, &CommandHistory::notifyCommandExecuted, this, &KEBApp::notifyCommandExecuted);

    GlobalBookmarkManager::self()->createManager(m_bookmarksFilename, m_dbusObjectName, m_cmdHistory);
    connect(GlobalBookmarkManager::self(), &GlobalBookmarkManager::error, this, &KEBApp::slotManagerError);

    s_topLevel = this;

    createActions();
    if (m_browser)
        createGUI();
    else
        createGUI(QStringLiteral("keditbookmarks-genui.rc"));

    connect(qApp->clipboard(), &QClipboard::dataChanged, this, &KEBApp::slotClipboardDataChanged);

    m_canPaste = false;

    mBookmarkListView = new BookmarkListView();
    mBookmarkListView->setModel(GlobalBookmarkManager::self()->model());
    mBookmarkListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mBookmarkListView->loadColumnSetting();
    mBookmarkListView->loadFoldedState();

    KViewSearchLineWidget *searchline = new KViewSearchLineWidget(mBookmarkListView, this);

    mBookmarkFolderView = new BookmarkFolderView(mBookmarkListView, this);
    mBookmarkFolderView->expandAll();

    QWidget *rightSide = new QWidget;
    QVBoxLayout *listLayout = new QVBoxLayout(rightSide);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->addWidget(searchline);
    listLayout->addWidget(mBookmarkListView);

    m_bkinfo = new BookmarkInfoWidget(mBookmarkListView, GlobalBookmarkManager::self()->model());

    listLayout->addWidget(m_bkinfo);

    QSplitter *hsplitter = new QSplitter(this);
    hsplitter->setOrientation(Qt::Horizontal);
    hsplitter->addWidget(mBookmarkFolderView);
    hsplitter->addWidget(rightSide);
    hsplitter->setStretchFactor(1, 1);

    setCentralWidget(hsplitter);

    slotClipboardDataChanged();
    setAutoSaveSettings();

    connect(mBookmarkListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &KEBApp::selectionChanged);

    connect(mBookmarkFolderView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &KEBApp::selectionChanged);

    setCancelFavIconUpdatesEnabled(false);
    setCancelTestsEnabled(false);
    updateActions();
}

void KEBApp::expandAll()
{
    mBookmarkListView->expandAll();
}

void KEBApp::collapseAll()
{
    mBookmarkListView->collapseAll();
}

QString KEBApp::bookmarkFilename()
{
    return m_bookmarksFilename;
}

void KEBApp::reset(const QString &caption, const QString &bookmarksFileName)
{
    // FIXME check this code, probably should be model()->setRoot instead of resetModel()
    m_caption = caption;
    m_bookmarksFilename = bookmarksFileName;
    GlobalBookmarkManager::self()->createManager(m_bookmarksFilename,
                                                 m_dbusObjectName,
                                                 m_cmdHistory); // FIXME this is still a memory leak (iff called by slotLoad)
    GlobalBookmarkManager::self()->model()->resetModel();
    updateActions();
}

void KEBApp::startEdit(Column c)
{
    const QModelIndexList &list = mBookmarkListView->selectionModel()->selectedIndexes();
    QModelIndexList::const_iterator it, end;
    end = list.constEnd();
    for (it = list.constBegin(); it != end; ++it)
        if ((*it).column() == int(c) && (mBookmarkListView->model()->flags(*it) & Qt::ItemIsEditable))
            mBookmarkListView->edit(*it);
}

// FIXME clean up and remove unneeded things
SelcAbilities KEBApp::getSelectionAbilities() const
{
    SelcAbilities selctionAbilities;
    selctionAbilities.itemSelected = false;
    selctionAbilities.group = false;
    selctionAbilities.separator = false;
    selctionAbilities.urlIsEmpty = false;
    selctionAbilities.root = false;
    selctionAbilities.multiSelect = false;
    selctionAbilities.singleSelect = false;
    selctionAbilities.notEmpty = false;
    selctionAbilities.deleteEnabled = false;

    KBookmark nbk;
    QModelIndexList sel = mBookmarkListView->selectionModel()->selectedIndexes();
    int columnCount;
    if (sel.count()) {
        nbk = mBookmarkListView->bookmarkForIndex(sel.first());
        columnCount = mBookmarkListView->model()->columnCount();
    } else {
        sel = mBookmarkFolderView->selectionModel()->selectedIndexes();
        if (sel.count())
            nbk = mBookmarkFolderView->bookmarkForIndex(sel.first());
        columnCount = mBookmarkFolderView->model()->columnCount();
    }

    if (sel.count() > 0) {
        selctionAbilities.deleteEnabled = true;
        selctionAbilities.itemSelected = true;
        selctionAbilities.group = nbk.isGroup();
        selctionAbilities.separator = nbk.isSeparator();
        selctionAbilities.urlIsEmpty = nbk.url().isEmpty();
        selctionAbilities.root = nbk.address() == GlobalBookmarkManager::self()->root().address();
        selctionAbilities.multiSelect = (sel.count() > columnCount);
        selctionAbilities.singleSelect = (!selctionAbilities.multiSelect && selctionAbilities.itemSelected);
    }
    // FIXME check next line, if it actually works
    selctionAbilities.notEmpty = GlobalBookmarkManager::self()->root().first().hasParent();

    /*    //qCDebug(KEDITBOOKMARKS_LOG)
            <<"\nsa.itemSelected "<<selctionAbilities.itemSelected
            <<"\nsa.group        "<<selctionAbilities.group
            <<"\nsa.separator    "<<selctionAbilities.separator
            <<"\nsa.urlIsEmpty   "<<selctionAbilities.urlIsEmpty
            <<"\nsa.root         "<<selctionAbilities.root
            <<"\nsa.multiSelect  "<<selctionAbilities.multiSelect
            <<"\nsa.singleSelect "<<selctionAbilities.singleSelect
            <<"\nsa.deleteEnabled"<<selctionAbilities.deleteEnabled;
    */
    return selctionAbilities;
}

void KEBApp::setActionsEnabled(SelcAbilities sa)
{
    KActionCollection *coll = actionCollection();

    QStringList toEnable;

    if (sa.multiSelect || (sa.singleSelect && !sa.root))
        toEnable << QStringLiteral("edit_copy");

    if (sa.multiSelect || (sa.singleSelect && !sa.root && !sa.urlIsEmpty && !sa.group && !sa.separator))
        toEnable << QStringLiteral("openlink");

    if (!m_readOnly) {
        if (sa.notEmpty)
            toEnable << QStringLiteral("testall") << QStringLiteral("updateallfavicons");

        if (sa.deleteEnabled && (sa.multiSelect || (sa.singleSelect && !sa.root)))
            toEnable << QStringLiteral("delete") << QStringLiteral("edit_cut");

        if (sa.singleSelect)
            if (m_canPaste)
                toEnable << QStringLiteral("edit_paste");

        if (sa.multiSelect || (sa.singleSelect && !sa.root && !sa.urlIsEmpty && !sa.group && !sa.separator))
            toEnable << QStringLiteral("testlink") << QStringLiteral("updatefavicon");

        if (sa.singleSelect && !sa.root && !sa.separator) {
            toEnable << QStringLiteral("rename") << QStringLiteral("changeicon") << QStringLiteral("changecomment");
            if (!sa.group)
                toEnable << QStringLiteral("changeurl");
        }

        if (sa.singleSelect) {
            toEnable << QStringLiteral("newfolder") << QStringLiteral("newbookmark") << QStringLiteral("insertseparator");
            if (sa.group)
                toEnable << QStringLiteral("sort") << QStringLiteral("recursivesort") << QStringLiteral("setastoolbar");
        }
    }

    for (QStringList::const_iterator it = toEnable.constBegin(); it != toEnable.constEnd(); ++it) {
        ////qCDebug(KEDITBOOKMARKS_LOG) <<" enabling action "<<(*it);
        coll->action(*it)->setEnabled(true);
    }
}

KBookmark KEBApp::firstSelected() const
{
    const QModelIndexList &list = mBookmarkListView->selectionModel()->selectedIndexes();
    if (list.count()) // selection in main listview, return bookmark for firstSelected
        return mBookmarkListView->bookmarkForIndex(*list.constBegin());

    // no selection in main listview, fall back to selection in left tree
    const QModelIndexList &list2 = mBookmarkFolderView->selectionModel()->selectedIndexes();
    return mBookmarkFolderView->bookmarkForIndex(*list2.constBegin());
}

QString KEBApp::insertAddress() const
{
    KBookmark current = firstSelected();
    return (current.isGroup()) ? (current.address() + QStringLiteral("/0")) // FIXME internal representation used
                               : KBookmark::nextAddress(current.address());
}

static bool lessAddress(const QString &first, const QString &second)
{
    QString a = first;
    QString b = second;

    if (a == b)
        return false;

    QString error(QStringLiteral("ERROR"));
    if (a == error)
        return false;
    if (b == error)
        return true;

    a += QLatin1Char('/');
    b += QLatin1Char('/');

    uint aLast = 0;
    uint bLast = 0;
    uint aEnd = a.length();
    uint bEnd = b.length();
    // Each iteration checks one "/"-delimeted part of the address
    // "" is treated correctly
    while (true) {
        // Invariant: a[0 ... aLast] == b[0 ... bLast]
        if (aLast + 1 == aEnd) // The last position was the last slash
            return true; // That means a is shorter than b
        if (bLast + 1 == bEnd)
            return false;

        uint aNext = a.indexOf(QLatin1String("/"), aLast + 1);
        uint bNext = b.indexOf(QLatin1String("/"), bLast + 1);

        bool okay;
        uint aNum = QStringView(a).mid(aLast + 1, aNext - aLast - 1).toUInt(&okay);
        if (!okay)
            return false;
        uint bNum = QStringView(b).mid(bLast + 1, bNext - bLast - 1).toUInt(&okay);
        if (!okay)
            return true;

        if (aNum != bNum)
            return aNum < bNum;

        aLast = aNext;
        bLast = bNext;
    }
}

static bool lessBookmark(const KBookmark &first, const KBookmark &second) // FIXME Using internal represantation
{
    return lessAddress(first.address(), second.address());
}

KBookmark::List KEBApp::allBookmarks() const
{
    KBookmark::List bookmarks;
    selectedBookmarksExpandedHelper(GlobalBookmarkManager::self()->root(), bookmarks);
    return bookmarks;
}

KBookmark::List KEBApp::selectedBookmarks() const
{
    KBookmark::List bookmarks;
    const QModelIndexList &list = mBookmarkListView->selectionModel()->selectedIndexes();
    if (!list.isEmpty()) {
        QModelIndexList::const_iterator it, end;
        end = list.constEnd();
        for (it = list.constBegin(); it != end; ++it) {
            if ((*it).column() != 0)
                continue;
            KBookmark bk = mBookmarkListView->bookmarkModel()->bookmarkForIndex(*it);
            ;
            if (bk.address() != GlobalBookmarkManager::self()->root().address())
                bookmarks.push_back(bk);
        }
        std::sort(bookmarks.begin(), bookmarks.end(), lessBookmark);
    } else {
        bookmarks.push_back(firstSelected());
    }

    return bookmarks;
}

KBookmark::List KEBApp::selectedBookmarksExpanded() const
{
    KBookmark::List bookmarks = selectedBookmarks();
    KBookmark::List result;
    KBookmark::List::const_iterator it, end;
    end = bookmarks.constEnd();
    for (it = bookmarks.constBegin(); it != end; ++it) {
        selectedBookmarksExpandedHelper(*it, result);
    }
    return result;
}

void KEBApp::selectedBookmarksExpandedHelper(const KBookmark &bk, KBookmark::List &bookmarks) const
{
    // FIXME in which order parents should ideally be: parent then child
    // or child and then parents
    if (bk.isGroup()) {
        KBookmarkGroup parent = bk.toGroup();
        KBookmark child = parent.first();
        while (!child.isNull()) {
            selectedBookmarksExpandedHelper(child, bookmarks);
            child = parent.next(child);
        }
    } else {
        bookmarks.push_back(bk);
    }
}

KEBApp::~KEBApp()
{
    // Save again, just in case the user expanded/collapsed folders (#131127)
    GlobalBookmarkManager::self()->notifyManagers();

    s_topLevel = nullptr;
    delete m_cmdHistory;
    delete m_actionsImpl;
    delete mBookmarkListView;
    delete GlobalBookmarkManager::self();
}

void KEBApp::resetActions()
{
    stateChanged(QStringLiteral("disablestuff"));
    stateChanged(QStringLiteral("normal"));

    if (!m_readOnly)
        stateChanged(QStringLiteral("notreadonly"));
}

void KEBApp::selectionChanged()
{
    updateActions();
}

void KEBApp::updateActions()
{
    resetActions();
    setActionsEnabled(getSelectionAbilities());
}

void KEBApp::slotClipboardDataChanged()
{
    // //qCDebug(KEDITBOOKMARKS_LOG) << "KEBApp::slotClipboardDataChanged";
    if (!m_readOnly) {
        m_canPaste = KBookmark::List::canDecode(QApplication::clipboard()->mimeData());
        updateActions();
    }
}

/* -------------------------- */

void KEBApp::notifyCommandExecuted()
{
    // //qCDebug(KEDITBOOKMARKS_LOG) << "KEBApp::notifyCommandExecuted()";
    updateActions();
}

/* -------------------------- */

void KEBApp::slotConfigureToolbars()
{
    // PORT TO QT5 saveMainWindowSettings(KConfigGroup( KSharedConfig::openConfig(), "MainWindow") );
    KEditToolBar dlg(actionCollection(), this);
    connect(&dlg, &KEditToolBar::newToolBarConfig, this, &KEBApp::slotNewToolbarConfig);
    dlg.exec();
}

void KEBApp::slotNewToolbarConfig()
{
    // called when OK or Apply is clicked
    createGUI();
    applyMainWindowSettings(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("MainWindow")));
}

void KEBApp::slotManagerError(const QString &errorMessage)
{
    KMessageBox::error(this, errorMessage);
}

/* -------------------------- */

#include "moc_toplevel.cpp"
