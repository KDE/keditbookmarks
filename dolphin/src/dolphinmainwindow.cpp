/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Stefan Monov <logixoul@gmail.com>               *
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "dolphinmainwindow.h"

#include "config-nepomuk.h"

#include "dolphinapplication.h"
#include "dolphinnewmenu.h"
#include "dolphinsettings.h"
#include "dolphinsettingsdialog.h"
#include "dolphinstatusbar.h"
#include "dolphinviewcontainer.h"
#include "infosidebarpage.h"
#include "metadatawidget.h"
#include "mainwindowadaptor.h"
#include "terminalsidebarpage.h"
#include "treeviewsidebarpage.h"
#include "kurlnavigator.h"
#include "viewpropertiesdialog.h"
#include "viewproperties.h"
#include "kfileplacesmodel.h"
#include "kfileplacesview.h"

#include "dolphin_generalsettings.h"

#include <kaction.h>
#include <kactioncollection.h>
#include <kconfig.h>
#include <kdesktopfile.h>
#include <kdeversion.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kio/deletejob.h>
#include <kio/renamedialog.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <konqmimedata.h>
#include <konq_operations.h>
#include <kpropertiesdialog.h>
#include <kprotocolinfo.h>
#include <ktoggleaction.h>
#include <krun.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <kstandardaction.h>
#include <kurl.h>

#include <QtGui/QKeyEvent>
#include <QtGui/QClipboard>
#include <QtGui/QSplitter>
#include <QtGui/QDockWidget>

DolphinMainWindow::DolphinMainWindow(int id) :
    KXmlGuiWindow(0),
    m_newMenu(0),
    m_splitter(0),
    m_activeViewContainer(0),
    m_id(id)
{
    setObjectName("Dolphin");
    m_viewContainer[PrimaryIdx] = 0;
    m_viewContainer[SecondaryIdx] = 0;

    new MainWindowAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QString("/dolphin/MainWindow%1").arg(m_id), this);

    KonqUndoManager::incRef();

    KonqUndoManager* undoManager = KonqUndoManager::self();
    undoManager->setUiInterface(new UndoUiInterface(this));

    connect(undoManager, SIGNAL(undoAvailable(bool)),
            this, SLOT(slotUndoAvailable(bool)));
    connect(undoManager, SIGNAL(undoTextChanged(const QString&)),
            this, SLOT(slotUndoTextChanged(const QString&)));
}

DolphinMainWindow::~DolphinMainWindow()
{
    KonqUndoManager::decRef();
    DolphinApplication::app()->removeMainWindow(this);
}

void DolphinMainWindow::setActiveView(DolphinViewContainer* view)
{
    Q_ASSERT((view == m_viewContainer[PrimaryIdx]) || (view == m_viewContainer[SecondaryIdx]));
    if (m_activeViewContainer == view) {
        return;
    }

    m_activeViewContainer->setActive(false);
    m_activeViewContainer = view;
    m_activeViewContainer->setActive(true);

    updateHistory();
    updateEditActions();
    updateViewActions();
    updateGoActions();

    setCaption(m_activeViewContainer->url().fileName());

    emit activeViewChanged();
}

void DolphinMainWindow::dropUrls(const KUrl::List& urls,
                                 const KUrl& destination)
{
    Qt::DropAction action = Qt::CopyAction;

    Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
    const bool shiftPressed   = modifier & Qt::ShiftModifier;
    const bool controlPressed = modifier & Qt::ControlModifier;
    if (shiftPressed && controlPressed) {
        // shortcut for 'Link Here' is used
        action = Qt::LinkAction;
    } else if (shiftPressed) {
        // shortcut for 'Move Here' is used
        action = Qt::MoveAction;
    } else if (controlPressed) {
        // shortcut for 'Copy Here' is used
        action = Qt::CopyAction;
    } else {
        // open a context menu which offers the following actions:
        // - Move Here
        // - Copy Here
        // - Link Here
        // - Cancel

        KMenu popup(this);

        QString seq = QKeySequence(Qt::ShiftModifier).toString();
        seq.chop(1); // chop superfluous '+'
        QAction* moveAction = popup.addAction(KIcon("goto-page"),
                                              i18n("&Move Here") + '\t' + seq);

        seq = QKeySequence(Qt::ControlModifier).toString();
        seq.chop(1);
        QAction* copyAction = popup.addAction(KIcon("edit-copy"),
                                              i18n("&Copy Here") + '\t' + seq);

        seq = QKeySequence(Qt::ControlModifier + Qt::ShiftModifier).toString();
        seq.chop(1);
        QAction* linkAction = popup.addAction(KIcon("www"),
                                              i18n("&Link Here") + '\t' + seq);

        popup.addSeparator();
        popup.addAction(KIcon("process-stop"), i18n("Cancel"));

        QAction* activatedAction = popup.exec(QCursor::pos());
        if (activatedAction == moveAction) {
            action = Qt::MoveAction;
        } else if (activatedAction == copyAction) {
            action = Qt::CopyAction;
        } else if (activatedAction == linkAction) {
            action = Qt::LinkAction;
        } else {
            return;
        }
    }

    switch (action) {
    case Qt::MoveAction:
        moveUrls(urls, destination);
        break;

    case Qt::CopyAction:
        copyUrls(urls, destination);
        break;

    case Qt::LinkAction:
        linkUrls(urls, destination);
        break;

    default:
        break;
    }
}

void DolphinMainWindow::rename(const KUrl& oldUrl, const KUrl& newUrl)
{
    clearStatusBar();
    KonqOperations::rename(this, oldUrl, newUrl);
    m_undoCommandTypes.append(KonqUndoManager::RENAME);
}

void DolphinMainWindow::refreshViews()
{
    Q_ASSERT(m_viewContainer[PrimaryIdx] != 0);

    // remember the current active view, as because of
    // the refreshing the active view might change to
    // the secondary view
    DolphinViewContainer* activeViewContainer = m_activeViewContainer;

    m_viewContainer[PrimaryIdx]->view()->refresh();
    if (m_viewContainer[SecondaryIdx] != 0) {
        m_viewContainer[SecondaryIdx]->view()->refresh();
    }

    setActiveView(activeViewContainer);
}

void DolphinMainWindow::changeUrl(const KUrl& url)
{
    if (activeViewContainer() != 0) {
        activeViewContainer()->setUrl(url);
        updateEditActions();
        updateViewActions();
        updateGoActions();
        setCaption(url.fileName());
        emit urlChanged(url);
    }
}

void DolphinMainWindow::changeSelection(const KFileItemList& selection)
{
    activeViewContainer()->view()->changeSelection(selection);
}

void DolphinMainWindow::slotViewModeChanged()
{
    updateViewActions();
}

void DolphinMainWindow::slotShowPreviewChanged()
{
    // It is not enough to update the 'Show Preview' action, also
    // the 'Zoom In' and 'Zoom Out' actions must be adapted.
    updateViewActions();
}

void DolphinMainWindow::slotShowHiddenFilesChanged()
{
    KToggleAction* showHiddenFilesAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_hidden_files"));
    const DolphinView* view = m_activeViewContainer->view();
    showHiddenFilesAction->setChecked(view->showHiddenFiles());
}

void DolphinMainWindow::slotCategorizedSortingChanged()
{
    KToggleAction* categorizedSortingAction =
        static_cast<KToggleAction*>(actionCollection()->action("categorized"));
    const DolphinView* view = m_activeViewContainer->view();
    categorizedSortingAction->setChecked(view->categorizedSorting());
    categorizedSortingAction->setEnabled(view->supportsCategorizedSorting());
}

void DolphinMainWindow::slotSortingChanged(DolphinView::Sorting sorting)
{
    QAction* action = 0;
    switch (sorting) {
    case DolphinView::SortByName:
        action = actionCollection()->action("by_name");
        break;
    case DolphinView::SortBySize:
        action = actionCollection()->action("by_size");
        break;
    case DolphinView::SortByDate:
        action = actionCollection()->action("by_date");
        break;
    case DolphinView::SortByPermissions:
        action = actionCollection()->action("by_permissions");
        break;
    case DolphinView::SortByOwner:
        action = actionCollection()->action("by_owner");
        break;
    case DolphinView::SortByGroup:
        action = actionCollection()->action("by_group");
        break;
    case DolphinView::SortByType:
        action = actionCollection()->action("by_type");
    default:
        break;
    }

    if (action != 0) {
        KToggleAction* toggleAction = static_cast<KToggleAction*>(action);
        toggleAction->setChecked(true);
    }
}

void DolphinMainWindow::slotSortOrderChanged(Qt::SortOrder order)
{
    KToggleAction* descending = static_cast<KToggleAction*>(actionCollection()->action("descending"));
    const bool sortDescending = (order == Qt::DescendingOrder);
    descending->setChecked(sortDescending);
}

void DolphinMainWindow::slotAdditionalInfoChanged(KFileItemDelegate::AdditionalInformation info)
{
    QAction* action = 0;
    switch (info) {
    case KFileItemDelegate::FriendlyMimeType:
        action = actionCollection()->action("show_mime_info");
        break;
    case KFileItemDelegate::Size:
        action = actionCollection()->action("show_size_info");
        break;
    case KFileItemDelegate::ModificationTime:
        action = actionCollection()->action("show_date_info");
        break;
    case KFileItemDelegate::NoInformation:
    default:
        action = actionCollection()->action("clear_info");
        break;
    }

    if (action != 0) {
        KToggleAction* toggleAction = static_cast<KToggleAction*>(action);
        toggleAction->setChecked(true);

        QActionGroup* group = toggleAction->actionGroup();
        Q_ASSERT(group != 0);
        const DolphinView* view = m_activeViewContainer->view();
        group->setEnabled(view->mode() == DolphinView::IconsView);
    }
}

void DolphinMainWindow::slotSelectionChanged(const KFileItemList& selection)
{
    updateEditActions();

    Q_ASSERT(m_viewContainer[PrimaryIdx] != 0);
    int selectedUrlsCount = m_viewContainer[PrimaryIdx]->view()->selectedUrls().count();
    if (m_viewContainer[SecondaryIdx] != 0) {
        selectedUrlsCount += m_viewContainer[SecondaryIdx]->view()->selectedUrls().count();
    }

    QAction* compareFilesAction = actionCollection()->action("compare_files");
    compareFilesAction->setEnabled(selectedUrlsCount == 2);

    m_activeViewContainer->updateStatusBar();

    emit selectionChanged(selection);
}

void DolphinMainWindow::slotRequestItemInfo(const KUrl& url)
{
    emit requestItemInfo(url);
}

void DolphinMainWindow::slotHistoryChanged()
{
    updateHistory();
}

void DolphinMainWindow::updateFilterBarAction(bool show)
{
    KToggleAction* showFilterBarAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_filter_bar"));
    showFilterBarAction->setChecked(show);
}

void DolphinMainWindow::openNewMainWindow()
{
    DolphinApplication::app()->createMainWindow()->show();
}

void DolphinMainWindow::toggleActiveView()
{
    if (m_activeViewContainer == m_viewContainer[PrimaryIdx]) {
        setActiveView(m_viewContainer[SecondaryIdx]);
    } else {
        setActiveView(m_viewContainer[PrimaryIdx]);
    }
}

void DolphinMainWindow::closeEvent(QCloseEvent* event)
{
    DolphinSettings& settings = DolphinSettings::instance();
    GeneralSettings* generalSettings = settings.generalSettings();
    generalSettings->setFirstRun(false);

    settings.save();

    KXmlGuiWindow::closeEvent(event);
}

void DolphinMainWindow::saveProperties(KConfig* config)
{
    KConfigGroup primaryView = config->group("Primary view");
    primaryView.writeEntry("Url", m_viewContainer[PrimaryIdx]->url().url());
    primaryView.writeEntry("Editable Url", m_viewContainer[PrimaryIdx]->isUrlEditable());
    if (m_viewContainer[SecondaryIdx] != 0) {
        KConfigGroup secondaryView = config->group("Secondary view");
        secondaryView.writeEntry("Url", m_viewContainer[SecondaryIdx]->url().url());
        secondaryView.writeEntry("Editable Url", m_viewContainer[SecondaryIdx]->isUrlEditable());
    }
}

void DolphinMainWindow::readProperties(KConfig* config)
{
    const KConfigGroup primaryViewGroup = config->group("Primary view");
    m_viewContainer[PrimaryIdx]->setUrl(primaryViewGroup.readEntry("Url"));
    bool editable = primaryViewGroup.readEntry("Editable Url", false);
    m_viewContainer[PrimaryIdx]->urlNavigator()->setUrlEditable(editable);

    if (config->hasGroup("Secondary view")) {
        const KConfigGroup secondaryViewGroup = config->group("Secondary view");
        if (m_viewContainer[PrimaryIdx] == 0) {
            toggleSplitView();
        }
        m_viewContainer[PrimaryIdx]->setUrl(secondaryViewGroup.readEntry("Url"));
        editable = secondaryViewGroup.readEntry("Editable Url", false);
        m_viewContainer[PrimaryIdx]->urlNavigator()->setUrlEditable(editable);
    } else if (m_viewContainer[SecondaryIdx] != 0) {
        toggleSplitView();
    }
}

void DolphinMainWindow::updateNewMenu()
{
    m_newMenu->slotCheckUpToDate();
    m_newMenu->setPopupFiles(activeViewContainer()->url());
}

void DolphinMainWindow::rename()
{
    clearStatusBar();
    m_activeViewContainer->renameSelectedItems();
}

void DolphinMainWindow::moveToTrash()
{
    clearStatusBar();
    const KUrl::List selectedUrls = m_activeViewContainer->view()->selectedUrls();
    KonqOperations::del(this, KonqOperations::TRASH, selectedUrls);
    m_undoCommandTypes.append(KonqUndoManager::TRASH);
}

void DolphinMainWindow::deleteItems()
{
    clearStatusBar();

    const KUrl::List list = m_activeViewContainer->view()->selectedUrls();
    const bool del = KonqOperations::askDeleteConfirmation(list,
                     KonqOperations::DEL,
                     KonqOperations::DEFAULT_CONFIRMATION,
                     this);

    if (del) {
        KIO::Job* job = KIO::del(list);
        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotHandleJobError(KJob*)));
        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotDeleteFileFinished(KJob*)));
    }
}

void DolphinMainWindow::properties()
{
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    KPropertiesDialog dialog(list, this);
    dialog.exec();
}

void DolphinMainWindow::quit()
{
    close();
}

void DolphinMainWindow::slotHandleJobError(KJob* job)
{
    if (job->error() != 0) {
        DolphinStatusBar* statusBar = m_activeViewContainer->statusBar();
        statusBar->setMessage(job->errorString(),
                              DolphinStatusBar::Error);
    }
}

void DolphinMainWindow::slotDeleteFileFinished(KJob* job)
{
    if (job->error() == 0) {
        DolphinStatusBar* statusBar = m_activeViewContainer->statusBar();
        statusBar->setMessage(i18n("Delete operation completed."),
                              DolphinStatusBar::OperationCompleted);
    }
}

void DolphinMainWindow::slotUndoAvailable(bool available)
{
    QAction* undoAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Undo));
    if (undoAction != 0) {
        undoAction->setEnabled(available);
    }

    if (available && (m_undoCommandTypes.count() > 0)) {
        const KonqUndoManager::CommandType command = m_undoCommandTypes.takeFirst();
        DolphinStatusBar* statusBar = m_activeViewContainer->statusBar();
        switch (command) {
        case KonqUndoManager::COPY:
            statusBar->setMessage(i18n("Copy operation completed."),
                                  DolphinStatusBar::OperationCompleted);
            break;
        case KonqUndoManager::MOVE:
            statusBar->setMessage(i18n("Move operation completed."),
                                  DolphinStatusBar::OperationCompleted);
            break;
        case KonqUndoManager::LINK:
            statusBar->setMessage(i18n("Link operation completed."),
                                  DolphinStatusBar::OperationCompleted);
            break;
        case KonqUndoManager::TRASH:
            statusBar->setMessage(i18n("Move to trash operation completed."),
                                  DolphinStatusBar::OperationCompleted);
            break;
        case KonqUndoManager::RENAME:
            statusBar->setMessage(i18n("Renaming operation completed."),
                                  DolphinStatusBar::OperationCompleted);
            break;

        case KonqUndoManager::MKDIR:
            statusBar->setMessage(i18n("Created folder."),
                                  DolphinStatusBar::OperationCompleted);
            break;

        default:
            break;
        }

    }
}

void DolphinMainWindow::slotUndoTextChanged(const QString& text)
{
    QAction* undoAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Undo));
    if (undoAction != 0) {
        undoAction->setText(text);
    }
}

void DolphinMainWindow::undo()
{
    clearStatusBar();
    KonqUndoManager::self()->undo();
}

void DolphinMainWindow::cut()
{
    QMimeData* mimeData = new QMimeData();
    const KUrl::List kdeUrls = m_activeViewContainer->view()->selectedUrls();
    const KUrl::List mostLocalUrls;
    KonqMimeData::populateMimeData(mimeData, kdeUrls, mostLocalUrls, true);
    QApplication::clipboard()->setMimeData(mimeData);
}

void DolphinMainWindow::copy()
{
    QMimeData* mimeData = new QMimeData();
    const KUrl::List kdeUrls = m_activeViewContainer->view()->selectedUrls();
    const KUrl::List mostLocalUrls;
    KonqMimeData::populateMimeData(mimeData, kdeUrls, mostLocalUrls, false);

    QApplication::clipboard()->setMimeData(mimeData);
}

void DolphinMainWindow::paste()
{
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();

    clearStatusBar();

    const KUrl::List sourceUrls = KUrl::List::fromMimeData(mimeData);

    // per default the pasting is done into the current Url of the view
    KUrl destUrl(m_activeViewContainer->url());

    // check whether the pasting should be done into a selected directory
    KUrl::List selectedUrls = m_activeViewContainer->view()->selectedUrls();
    if (selectedUrls.count() == 1) {
        const KFileItem fileItem(S_IFDIR,
                                 KFileItem::Unknown,
                                 selectedUrls.first(),
                                 true);
        if (fileItem.isDir()) {
            // only one item is selected which is a directory, hence paste
            // into this directory
            destUrl = selectedUrls.first();
        }
    }

    if (KonqMimeData::decodeIsCutSelection(mimeData)) {
        moveUrls(sourceUrls, destUrl);
        clipboard->clear();
    } else {
        copyUrls(sourceUrls, destUrl);
    }
}

void DolphinMainWindow::updatePasteAction()
{
    QAction* pasteAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Paste));
    if (pasteAction == 0) {
        return;
    }

    QString text(i18n("Paste"));
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();

    KUrl::List urls = KUrl::List::fromMimeData(mimeData);
    if (!urls.isEmpty()) {
        pasteAction->setEnabled(true);

        pasteAction->setText(i18np("Paste One File", "Paste %1 Files", urls.count()));
    } else {
        pasteAction->setEnabled(false);
        pasteAction->setText(i18n("Paste"));
    }

    if (pasteAction->isEnabled()) {
        KUrl::List urls = m_activeViewContainer->view()->selectedUrls();
        const uint count = urls.count();
        if (count > 1) {
            // pasting should not be allowed when more than one file
            // is selected
            pasteAction->setEnabled(false);
        } else if (count == 1) {
            // Only one file is selected. Pasting is only allowed if this
            // file is a directory.
            // TODO: this doesn't work with remote protocols; instead we need a
            // m_activeViewContainer->selectedFileItems() to get the real KFileItems
            const KFileItem fileItem(S_IFDIR,
                                     KFileItem::Unknown,
                                     urls.first(),
                                     true);
            pasteAction->setEnabled(fileItem.isDir());
        }
    }
}

void DolphinMainWindow::selectAll()
{
    clearStatusBar();
    m_activeViewContainer->view()->selectAll();
}

void DolphinMainWindow::invertSelection()
{
    clearStatusBar();
    m_activeViewContainer->view()->invertSelection();
}
void DolphinMainWindow::setIconsView()
{
    m_activeViewContainer->view()->setMode(DolphinView::IconsView);
}

void DolphinMainWindow::setDetailsView()
{
    m_activeViewContainer->view()->setMode(DolphinView::DetailsView);
}

void DolphinMainWindow::setColumnView()
{
    m_activeViewContainer->view()->setMode(DolphinView::ColumnView);
}

void DolphinMainWindow::sortByName()
{
    m_activeViewContainer->view()->setSorting(DolphinView::SortByName);
}

void DolphinMainWindow::sortBySize()
{
    m_activeViewContainer->view()->setSorting(DolphinView::SortBySize);
}

void DolphinMainWindow::sortByDate()
{
    m_activeViewContainer->view()->setSorting(DolphinView::SortByDate);
}

void DolphinMainWindow::sortByPermissions()
{
    m_activeViewContainer->view()->setSorting(DolphinView::SortByPermissions);
}

void DolphinMainWindow::sortByOwner()
{
    m_activeViewContainer->view()->setSorting(DolphinView::SortByOwner);
}

void DolphinMainWindow::sortByGroup()
{
    m_activeViewContainer->view()->setSorting(DolphinView::SortByGroup);
}

void DolphinMainWindow::sortByType()
{
    m_activeViewContainer->view()->setSorting(DolphinView::SortByType);
}

void DolphinMainWindow::toggleSortOrder()
{
    DolphinView* view = m_activeViewContainer->view();
    const Qt::SortOrder order = (view->sortOrder() == Qt::AscendingOrder) ?
                                Qt::DescendingOrder :
                                Qt::AscendingOrder;
    view->setSortOrder(order);
}

void DolphinMainWindow::toggleSortCategorization()
{
    DolphinView* view = m_activeViewContainer->view();
    const bool categorizedSorting = view->categorizedSorting();
    view->setCategorizedSorting(!categorizedSorting);
}

void DolphinMainWindow::clearInfo()
{
    m_activeViewContainer->view()->setAdditionalInfo(KFileItemDelegate::NoInformation);
}

void DolphinMainWindow::showMimeInfo()
{
    clearStatusBar();
    m_activeViewContainer->view()->setAdditionalInfo(KFileItemDelegate::FriendlyMimeType);
}

void DolphinMainWindow::showSizeInfo()
{
    clearStatusBar();
    m_activeViewContainer->view()->setAdditionalInfo(KFileItemDelegate::Size);
}

void DolphinMainWindow::showDateInfo()
{
    clearStatusBar();
    m_activeViewContainer->view()->setAdditionalInfo(KFileItemDelegate::ModificationTime);
}

void DolphinMainWindow::toggleSplitView()
{
    if (m_viewContainer[SecondaryIdx] == 0) {
        const int newWidth = (m_viewContainer[PrimaryIdx]->width() - m_splitter->handleWidth()) / 2;
        // create a secondary view
        const DolphinView* view = m_viewContainer[PrimaryIdx]->view();
        m_viewContainer[SecondaryIdx] = new DolphinViewContainer(this,
                                                                 0,
                                                                 view->rootUrl(),
                                                                 view->mode(),
                                                                 view->showHiddenFiles());
        connectViewSignals(SecondaryIdx);
        m_splitter->addWidget(m_viewContainer[SecondaryIdx]);
        m_splitter->setSizes(QList<int>() << newWidth << newWidth);
        m_viewContainer[SecondaryIdx]->view()->reload();
        m_viewContainer[SecondaryIdx]->setActive(false);
        m_viewContainer[SecondaryIdx]->show();
    } else {
        // remove secondary view
        m_viewContainer[SecondaryIdx]->close();
        m_viewContainer[SecondaryIdx]->deleteLater();
        m_viewContainer[SecondaryIdx] = 0;
    }

    setActiveView(m_viewContainer[PrimaryIdx]);
    updateViewActions();
    emit activeViewChanged();
}

void DolphinMainWindow::reloadView()
{
    clearStatusBar();
    m_activeViewContainer->view()->reload();
}

void DolphinMainWindow::stopLoading()
{}

void DolphinMainWindow::togglePreview()
{
    clearStatusBar();

    const KToggleAction* showPreviewAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_preview"));
    const bool show = showPreviewAction->isChecked();
    m_activeViewContainer->view()->setShowPreview(show);
}

void DolphinMainWindow::toggleShowHiddenFiles()
{
    clearStatusBar();

    const KToggleAction* showHiddenFilesAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_hidden_files"));
    const bool show = showHiddenFilesAction->isChecked();
    m_activeViewContainer->view()->setShowHiddenFiles(show);
}

void DolphinMainWindow::toggleFilterBarVisibility()
{
    const KToggleAction* showFilterBarAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_filter_bar"));
    const bool show = showFilterBarAction->isChecked();
    m_activeViewContainer->showFilterBar(show);
}

void DolphinMainWindow::zoomIn()
{
    m_activeViewContainer->view()->zoomIn();
    updateViewActions();
}

void DolphinMainWindow::zoomOut()
{
    m_activeViewContainer->view()->zoomOut();
    updateViewActions();
}

void DolphinMainWindow::toggleEditLocation()
{
    clearStatusBar();

    KToggleAction* action = static_cast<KToggleAction*>(actionCollection()->action("editable_location"));

    bool editOrBrowse = action->isChecked();
    KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
    urlNavigator->setUrlEditable(editOrBrowse);
}

void DolphinMainWindow::editLocation()
{
    m_activeViewContainer->urlNavigator()->setUrlEditable(true);
}

void DolphinMainWindow::adjustViewProperties()
{
    clearStatusBar();
    ViewPropertiesDialog dlg(m_activeViewContainer->view());
    dlg.exec();
}

void DolphinMainWindow::goBack()
{
    clearStatusBar();
    m_activeViewContainer->urlNavigator()->goBack();
}

void DolphinMainWindow::goForward()
{
    clearStatusBar();
    m_activeViewContainer->urlNavigator()->goForward();
}

void DolphinMainWindow::goUp()
{
    clearStatusBar();
    m_activeViewContainer->urlNavigator()->goUp();
}

void DolphinMainWindow::goHome()
{
    clearStatusBar();
    m_activeViewContainer->urlNavigator()->goHome();
}

void DolphinMainWindow::findFile()
{
    KRun::run("kfind", m_activeViewContainer->url(), this);
}

void DolphinMainWindow::compareFiles()
{
    // The method is only invoked if exactly 2 files have
    // been selected. The selected files may be:
    // - both in the primary view
    // - both in the secondary view
    // - one in the primary view and the other in the secondary
    //   view
    Q_ASSERT(m_viewContainer[PrimaryIdx] != 0);

    KUrl urlA;
    KUrl urlB;
    KUrl::List urls = m_viewContainer[PrimaryIdx]->view()->selectedUrls();

    switch (urls.count()) {
    case 0: {
        Q_ASSERT(m_viewContainer[SecondaryIdx] != 0);
        urls = m_viewContainer[SecondaryIdx]->view()->selectedUrls();
        Q_ASSERT(urls.count() == 2);
        urlA = urls[0];
        urlB = urls[1];
        break;
    }

    case 1: {
        urlA = urls[0];
        Q_ASSERT(m_viewContainer[SecondaryIdx] != 0);
        urls = m_viewContainer[SecondaryIdx]->view()->selectedUrls();
        Q_ASSERT(urls.count() == 1);
        urlB = urls[0];
        break;
    }

    case 2: {
        urlA = urls[0];
        urlB = urls[1];
        break;
    }

    default: {
        // may not happen: compareFiles may only get invoked if 2
        // files are selected
        Q_ASSERT(false);
    }
    }

    QString command("kompare -c \"");
    command.append(urlA.pathOrUrl());
    command.append("\" \"");
    command.append(urlB.pathOrUrl());
    command.append('\"');
    KRun::runCommand(command, "Kompare", "kompare", this);

}

void DolphinMainWindow::editSettings()
{
    DolphinSettingsDialog dialog(this);
    dialog.exec();
}

void DolphinMainWindow::init()
{
    // Check whether Dolphin runs the first time. If yes then
    // a proper default window size is given at the end of DolphinMainWindow::init().
    GeneralSettings* generalSettings = DolphinSettings::instance().generalSettings();
    const bool firstRun = generalSettings->firstRun();
    if (firstRun) {
        generalSettings->setViewPropsTimestamp(QDateTime::currentDateTime());
    }

    setAcceptDrops(true);

    m_splitter = new QSplitter(this);

    DolphinSettings& settings = DolphinSettings::instance();

    setupActions();

    const KUrl& homeUrl = settings.generalSettings()->homeUrl();
    setCaption(homeUrl.fileName());
    ViewProperties props(homeUrl);
    m_viewContainer[PrimaryIdx] = new DolphinViewContainer(this,
                                                           m_splitter,
                                                           homeUrl,
                                                           props.viewMode(),
                                                           props.showHiddenFiles());

    m_activeViewContainer = m_viewContainer[PrimaryIdx];
    connectViewSignals(PrimaryIdx);
    m_viewContainer[PrimaryIdx]->view()->reload();
    m_viewContainer[PrimaryIdx]->show();

    setCentralWidget(m_splitter);
    setupDockWidgets();

    setupGUI(Keys | Save | Create | ToolBar);
    createGUI();

    stateChanged("new_file");
    setAutoSaveSettings();

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updatePasteAction()));
    updatePasteAction();
    updateGoActions();

    loadSettings();

    if (firstRun) {
        // assure a proper default size if Dolphin runs the first time
        resize(700, 500);
    }
#ifdef HAVE_NEPOMUK
    if (!MetaDataWidget::metaDataAvailable())
        activeViewContainer()->statusBar()->setMessage(i18n("Failed to contact Nepomuk service, annotation and tagging are disabled."), DolphinStatusBar::Error);
#endif

    emit urlChanged(homeUrl);
}

void DolphinMainWindow::loadSettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    updateSplitAction(settings->splitView());
    updateViewActions();
}

void DolphinMainWindow::setupActions()
{
    // setup 'File' menu
    m_newMenu = new DolphinNewMenu(this);
    KMenu* menu = m_newMenu->menu();
    menu->setTitle(i18n("Create New"));
    menu->setIcon(KIcon("document-new"));
    connect(menu, SIGNAL(aboutToShow()),
            this, SLOT(updateNewMenu()));

    QAction* newWindow = actionCollection()->addAction("new_window");
    newWindow->setIcon(KIcon("window-new"));
    newWindow->setText(i18n("New &Window"));
    newWindow->setShortcut(Qt::CTRL | Qt::Key_N);
    connect(newWindow, SIGNAL(triggered()), this, SLOT(openNewMainWindow()));

    QAction* rename = actionCollection()->addAction("rename");
    rename->setText(i18n("Rename..."));
    rename->setShortcut(Qt::Key_F2);
    connect(rename, SIGNAL(triggered()), this, SLOT(rename()));

    QAction* moveToTrash = actionCollection()->addAction("move_to_trash");
    moveToTrash->setText(i18n("Move to Trash"));
    moveToTrash->setIcon(KIcon("edit-trash"));
    moveToTrash->setShortcut(QKeySequence::Delete);
    connect(moveToTrash, SIGNAL(triggered()), this, SLOT(moveToTrash()));

    QAction* deleteAction = actionCollection()->addAction("delete");
    deleteAction->setText(i18n("Delete"));
    deleteAction->setShortcut(Qt::SHIFT | Qt::Key_Delete);
    deleteAction->setIcon(KIcon("edit-delete"));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteItems()));

    QAction* properties = actionCollection()->addAction("properties");
    properties->setText(i18n("Properties"));
    properties->setShortcut(Qt::ALT | Qt::Key_Return);
    connect(properties, SIGNAL(triggered()), this, SLOT(properties()));

    KStandardAction::quit(this, SLOT(quit()), actionCollection());

    // setup 'Edit' menu
    KStandardAction::undo(this,
                          SLOT(undo()),
                          actionCollection());

    KStandardAction::cut(this, SLOT(cut()), actionCollection());
    KStandardAction::copy(this, SLOT(copy()), actionCollection());
    KStandardAction::paste(this, SLOT(paste()), actionCollection());

    QAction* selectAll = actionCollection()->addAction("select_all");
    selectAll->setText(i18n("Select All"));
    selectAll->setShortcut(Qt::CTRL + Qt::Key_A);
    connect(selectAll, SIGNAL(triggered()), this, SLOT(selectAll()));

    QAction* invertSelection = actionCollection()->addAction("invert_selection");
    invertSelection->setText(i18n("Invert Selection"));
    invertSelection->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(invertSelection, SIGNAL(triggered()), this, SLOT(invertSelection()));

    // setup 'View' menu
    KStandardAction::zoomIn(this,
                            SLOT(zoomIn()),
                            actionCollection());

    KStandardAction::zoomOut(this,
                             SLOT(zoomOut()),
                             actionCollection());

    KToggleAction* iconsView = actionCollection()->add<KToggleAction>("icons");
    iconsView->setText(i18n("Icons"));
    iconsView->setShortcut(Qt::CTRL | Qt::Key_1);
    iconsView->setIcon(KIcon("fileview-icon"));
    connect(iconsView, SIGNAL(triggered()), this, SLOT(setIconsView()));

    KToggleAction* detailsView = actionCollection()->add<KToggleAction>("details");
    detailsView->setText(i18n("Details"));
    detailsView->setShortcut(Qt::CTRL | Qt::Key_2);
    detailsView->setIcon(KIcon("fileview-detailed"));
    connect(detailsView, SIGNAL(triggered()), this, SLOT(setDetailsView()));

    KToggleAction* columnView = actionCollection()->add<KToggleAction>("columns");
    columnView->setText(i18n("Columns"));
    columnView->setShortcut(Qt::CTRL | Qt::Key_3);
    columnView->setIcon(KIcon("fileview-column"));
    connect(columnView, SIGNAL(triggered()), this, SLOT(setColumnView()));

    QActionGroup* viewModeGroup = new QActionGroup(this);
    viewModeGroup->addAction(iconsView);
    viewModeGroup->addAction(detailsView);
    viewModeGroup->addAction(columnView);

    KToggleAction* sortByName = actionCollection()->add<KToggleAction>("by_name");
    sortByName->setText(i18n("By Name"));
    connect(sortByName, SIGNAL(triggered()), this, SLOT(sortByName()));

    KToggleAction* sortBySize = actionCollection()->add<KToggleAction>("by_size");
    sortBySize->setText(i18n("By Size"));
    connect(sortBySize, SIGNAL(triggered()), this, SLOT(sortBySize()));

    KToggleAction* sortByDate = actionCollection()->add<KToggleAction>("by_date");
    sortByDate->setText(i18n("By Date"));
    connect(sortByDate, SIGNAL(triggered()), this, SLOT(sortByDate()));

    KToggleAction* sortByPermissions = actionCollection()->add<KToggleAction>("by_permissions");
    sortByPermissions->setText(i18n("By Permissions"));
    connect(sortByPermissions, SIGNAL(triggered()), this, SLOT(sortByPermissions()));

    KToggleAction* sortByOwner = actionCollection()->add<KToggleAction>("by_owner");
    sortByOwner->setText(i18n("By Owner"));
    connect(sortByOwner, SIGNAL(triggered()), this, SLOT(sortByOwner()));

    KToggleAction* sortByGroup = actionCollection()->add<KToggleAction>("by_group");
    sortByGroup->setText(i18n("By Group"));
    connect(sortByGroup, SIGNAL(triggered()), this, SLOT(sortByGroup()));

    KToggleAction* sortByType = actionCollection()->add<KToggleAction>("by_type");
    sortByType->setText(i18n("By Type"));
    connect(sortByType, SIGNAL(triggered()), this, SLOT(sortByType()));

    QActionGroup* sortGroup = new QActionGroup(this);
    sortGroup->addAction(sortByName);
    sortGroup->addAction(sortBySize);
    sortGroup->addAction(sortByDate);
    sortGroup->addAction(sortByPermissions);
    sortGroup->addAction(sortByOwner);
    sortGroup->addAction(sortByGroup);
    sortGroup->addAction(sortByType);

    KToggleAction* sortDescending = actionCollection()->add<KToggleAction>("descending");
    sortDescending->setText(i18n("Descending"));
    connect(sortDescending, SIGNAL(triggered()), this, SLOT(toggleSortOrder()));

    KToggleAction* sortCategorized = actionCollection()->add<KToggleAction>("categorized");
    sortCategorized->setText(i18n("Show in Groups"));
    connect(sortCategorized, SIGNAL(triggered()), this, SLOT(toggleSortCategorization()));

    KToggleAction* clearInfo = actionCollection()->add<KToggleAction>("clear_info");
    clearInfo->setText(i18n("No Information"));
    connect(clearInfo, SIGNAL(triggered()), this, SLOT(clearInfo()));

    KToggleAction* showMimeInfo = actionCollection()->add<KToggleAction>("show_mime_info");
    showMimeInfo->setText(i18n("Type"));
    connect(showMimeInfo, SIGNAL(triggered()), this, SLOT(showMimeInfo()));

    KToggleAction* showSizeInfo = actionCollection()->add<KToggleAction>("show_size_info");
    showSizeInfo->setText(i18n("Size"));
    connect(showSizeInfo, SIGNAL(triggered()), this, SLOT(showSizeInfo()));

    KToggleAction* showDateInfo = actionCollection()->add<KToggleAction>("show_date_info");
    showDateInfo->setText(i18n("Date"));
    connect(showDateInfo, SIGNAL(triggered()), this, SLOT(showDateInfo()));

    QActionGroup* infoGroup = new QActionGroup(this);
    infoGroup->addAction(clearInfo);
    infoGroup->addAction(showMimeInfo);
    infoGroup->addAction(showSizeInfo);
    infoGroup->addAction(showDateInfo);

    KToggleAction* showPreview = actionCollection()->add<KToggleAction>("show_preview");
    showPreview->setText(i18n("Preview"));
    showPreview->setIcon(KIcon("fileview-preview"));
    connect(showPreview, SIGNAL(triggered()), this, SLOT(togglePreview()));

    KToggleAction* showHiddenFiles = actionCollection()->add<KToggleAction>("show_hidden_files");
    showHiddenFiles->setText(i18n("Show Hidden Files"));
    showHiddenFiles->setShortcut(Qt::ALT | Qt::Key_Period);
    connect(showHiddenFiles, SIGNAL(triggered()), this, SLOT(toggleShowHiddenFiles()));

    QAction* split = actionCollection()->addAction("split_view");
    split->setShortcut(Qt::Key_F10);
    updateSplitAction(false);
    connect(split, SIGNAL(triggered()), this, SLOT(toggleSplitView()));

    QAction* reload = actionCollection()->addAction("reload");
    reload->setText(i18n("Reload"));
    reload->setShortcut(Qt::Key_F5);
    reload->setIcon(KIcon("view-refresh"));
    connect(reload, SIGNAL(triggered()), this, SLOT(reloadView()));

    QAction* stop = actionCollection()->addAction("stop");
    stop->setText(i18n("Stop"));
    stop->setIcon(KIcon("process-stop"));
    connect(stop, SIGNAL(triggered()), this, SLOT(stopLoading()));

    // TODO: the URL navigator must emit a signal if the editable state has been
    // changed, so that the corresponding showFullLocation action is updated. Also
    // the naming "Show full Location" is currently confusing...
    KToggleAction* showFullLocation = actionCollection()->add<KToggleAction>("editable_location");
    showFullLocation->setText(i18n("Show Full Location"));
    showFullLocation->setShortcut(Qt::CTRL | Qt::Key_L);
    connect(showFullLocation, SIGNAL(triggered()), this, SLOT(toggleEditLocation()));

    QAction* editLocation = actionCollection()->addAction("edit_location");
    editLocation->setText(i18n("Edit Location"));
    editLocation->setShortcut(Qt::Key_F6);
    connect(editLocation, SIGNAL(triggered()), this, SLOT(editLocation()));

    QAction* adjustViewProps = actionCollection()->addAction("view_properties");
    adjustViewProps->setText(i18n("Adjust View Properties..."));
    connect(adjustViewProps, SIGNAL(triggered()), this, SLOT(adjustViewProperties()));

    // setup 'Go' menu
    KStandardAction::back(this, SLOT(goBack()), actionCollection());
    KStandardAction::forward(this, SLOT(goForward()), actionCollection());
    KStandardAction::up(this, SLOT(goUp()), actionCollection());
    KStandardAction::home(this, SLOT(goHome()), actionCollection());

    // setup 'Tools' menu
    QAction* findFile = actionCollection()->addAction("find_file");
    findFile->setText(i18n("Find File..."));
    findFile->setShortcut(Qt::CTRL | Qt::Key_F);
    findFile->setIcon(KIcon("file-find"));
    connect(findFile, SIGNAL(triggered()), this, SLOT(findFile()));

    KToggleAction* showFilterBar = actionCollection()->add<KToggleAction>("show_filter_bar");
    showFilterBar->setText(i18n("Show Filter Bar"));
    showFilterBar->setShortcut(Qt::CTRL | Qt::Key_I);
    connect(showFilterBar, SIGNAL(triggered()), this, SLOT(toggleFilterBarVisibility()));

    QAction* compareFiles = actionCollection()->addAction("compare_files");
    compareFiles->setText(i18n("Compare Files"));
    compareFiles->setIcon(KIcon("kompare"));
    compareFiles->setEnabled(false);
    connect(compareFiles, SIGNAL(triggered()), this, SLOT(compareFiles()));

    // setup 'Settings' menu
    KStandardAction::preferences(this, SLOT(editSettings()), actionCollection());
}

void DolphinMainWindow::setupDockWidgets()
{
    // setup "Information"
    QDockWidget* infoDock = new QDockWidget(i18n("Information"));
    infoDock->setObjectName("infoDock");
    infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    SidebarPage* infoWidget = new InfoSidebarPage(infoDock);
    infoDock->setWidget(infoWidget);

    infoDock->toggleViewAction()->setText(i18n("Information"));
    actionCollection()->addAction("show_info_panel", infoDock->toggleViewAction());

    addDockWidget(Qt::RightDockWidgetArea, infoDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            infoWidget, SLOT(setUrl(KUrl)));
    connect(this, SIGNAL(selectionChanged(KFileItemList)),
            infoWidget, SLOT(setSelection(KFileItemList)));
    connect(this, SIGNAL(requestItemInfo(KUrl)),
            infoWidget, SLOT(requestDelayedItemInfo(KUrl)));

    // setup "Tree View"
    QDockWidget* treeViewDock = new QDockWidget(i18n("Folders"));
    treeViewDock->setObjectName("treeViewDock");
    treeViewDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    TreeViewSidebarPage* treeWidget = new TreeViewSidebarPage(treeViewDock);
    treeViewDock->setWidget(treeWidget);

    treeViewDock->toggleViewAction()->setText(i18n("Folders"));
    actionCollection()->addAction("show_folders_panel", treeViewDock->toggleViewAction());

    addDockWidget(Qt::LeftDockWidgetArea, treeViewDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            treeWidget, SLOT(setUrl(KUrl)));
    connect(treeWidget, SIGNAL(changeUrl(KUrl)),
            this, SLOT(changeUrl(KUrl)));
    connect(treeWidget, SIGNAL(changeSelection(KFileItemList)),
            this, SLOT(changeSelection(KFileItemList)));
    connect(treeWidget, SIGNAL(urlsDropped(KUrl::List, KUrl)),
            this, SLOT(dropUrls(KUrl::List, KUrl)));

    // setup "Terminal"
    QDockWidget* terminalDock = new QDockWidget(i18n("Terminal"));
    terminalDock->setObjectName("terminalDock");
    terminalDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    SidebarPage* terminalWidget = new TerminalSidebarPage(terminalDock);
    terminalDock->setWidget(terminalWidget);

    terminalDock->toggleViewAction()->setText(i18n("Terminal"));
    actionCollection()->addAction("show_terminal_panel", terminalDock->toggleViewAction());

    addDockWidget(Qt::RightDockWidgetArea, terminalDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            terminalWidget, SLOT(setUrl(KUrl)));

    const bool firstRun = DolphinSettings::instance().generalSettings()->firstRun();
    if (firstRun) {
        infoDock->hide();
        treeViewDock->hide();
        terminalDock->hide();
    }

    QDockWidget *placesDock = new QDockWidget(i18n("Places"));
    placesDock->setObjectName("placesDock");
    placesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    KFilePlacesView *listView = new KFilePlacesView(placesDock);
    placesDock->setWidget(listView);
    listView->setModel(DolphinSettings::instance().placesModel());

    placesDock->toggleViewAction()->setText(i18n("Show Places Panel"));
    actionCollection()->addAction("show_places_panel", placesDock->toggleViewAction());

    addDockWidget(Qt::LeftDockWidgetArea, placesDock);
    connect(listView, SIGNAL(urlChanged(KUrl)),
            this, SLOT(changeUrl(KUrl)));
    connect(this, SIGNAL(urlChanged(KUrl)),
            listView, SLOT(setUrl(KUrl)));
}

void DolphinMainWindow::updateHistory()
{
    const KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
    const int index = urlNavigator->historyIndex();

    QAction* backAction = actionCollection()->action("go_back");
    if (backAction != 0) {
        backAction->setEnabled(index < urlNavigator->historySize() - 1);
    }

    QAction* forwardAction = actionCollection()->action("go_forward");
    if (forwardAction != 0) {
        forwardAction->setEnabled(index > 0);
    }
}

void DolphinMainWindow::updateEditActions()
{
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    if (list.isEmpty()) {
        stateChanged("has_no_selection");
    } else {
        stateChanged("has_selection");

        QAction* renameAction = actionCollection()->action("rename");
        if (renameAction != 0) {
            renameAction->setEnabled(list.count() >= 1);
        }

        bool enableMoveToTrash = true;

        KFileItemList::const_iterator it = list.begin();
        const KFileItemList::const_iterator end = list.end();
        while (it != end) {
            KFileItem* item = *it;
            const KUrl& url = item->url();
            // only enable the 'Move to Trash' action for local files
            if (!url.isLocalFile()) {
                enableMoveToTrash = false;
            }
            ++it;
        }

        QAction* moveToTrashAction = actionCollection()->action("move_to_trash");
        moveToTrashAction->setEnabled(enableMoveToTrash);
    }
    updatePasteAction();
}

void DolphinMainWindow::updateViewActions()
{
    const DolphinView* view = m_activeViewContainer->view();
    QAction* zoomInAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::ZoomIn));
    if (zoomInAction != 0) {
        zoomInAction->setEnabled(view->isZoomInPossible());
    }

    QAction* zoomOutAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::ZoomOut));
    if (zoomOutAction != 0) {
        zoomOutAction->setEnabled(view->isZoomOutPossible());
    }

    QAction* action = 0;
    switch (view->mode()) {
    case DolphinView::IconsView:
        action = actionCollection()->action("icons");
        break;
    case DolphinView::DetailsView:
        action = actionCollection()->action("details");
        break;
    case DolphinView::ColumnView:
        action = actionCollection()->action("columns");
        break;
    default:
        break;
    }

    if (action != 0) {
        KToggleAction* toggleAction = static_cast<KToggleAction*>(action);
        toggleAction->setChecked(true);
    }

    slotSortingChanged(view->sorting());
    slotSortOrderChanged(view->sortOrder());
    slotCategorizedSortingChanged();
    slotAdditionalInfoChanged(view->additionalInfo());

    KToggleAction* showFilterBarAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_filter_bar"));
    showFilterBarAction->setChecked(m_activeViewContainer->isFilterBarVisible());

    KToggleAction* showPreviewAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_preview"));
    showPreviewAction->setChecked(view->showPreview());

    KToggleAction* showHiddenFilesAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_hidden_files"));
    showHiddenFilesAction->setChecked(view->showHiddenFiles());

    updateSplitAction(m_viewContainer[SecondaryIdx] != 0);

    KToggleAction* editableLocactionAction =
        static_cast<KToggleAction*>(actionCollection()->action("editable_location"));
    const KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
    editableLocactionAction->setChecked(urlNavigator->isUrlEditable());
}

void DolphinMainWindow::updateGoActions()
{
    QAction* goUpAction = actionCollection()->action(KStandardAction::stdName(KStandardAction::Up));
    const KUrl& currentUrl = m_activeViewContainer->url();
    goUpAction->setEnabled(currentUrl.upUrl() != currentUrl);
}

void DolphinMainWindow::copyUrls(const KUrl::List& source, const KUrl& dest)
{
    KonqOperations::copy(this, KonqOperations::COPY, source, dest);
    m_undoCommandTypes.append(KonqUndoManager::COPY);
}

void DolphinMainWindow::moveUrls(const KUrl::List& source, const KUrl& dest)
{
    KonqOperations::copy(this, KonqOperations::MOVE, source, dest);
    m_undoCommandTypes.append(KonqUndoManager::MOVE);
}

void DolphinMainWindow::linkUrls(const KUrl::List& source, const KUrl& dest)
{
    KonqOperations::copy(this, KonqOperations::LINK, source, dest);
    m_undoCommandTypes.append(KonqUndoManager::LINK);
}

void DolphinMainWindow::clearStatusBar()
{
    m_activeViewContainer->statusBar()->clear();
}

void DolphinMainWindow::connectViewSignals(int viewIndex)
{
    DolphinViewContainer* container = m_viewContainer[viewIndex];
    connect(container, SIGNAL(showFilterBarChanged(bool)),
            this, SLOT(updateFilterBarAction(bool)));

    DolphinView* view = container->view();
    connect(view, SIGNAL(modeChanged()),
            this, SLOT(slotViewModeChanged()));
    connect(view, SIGNAL(showPreviewChanged()),
            this, SLOT(slotShowPreviewChanged()));
    connect(view, SIGNAL(showHiddenFilesChanged()),
            this, SLOT(slotShowHiddenFilesChanged()));
    connect(view, SIGNAL(categorizedSortingChanged()),
            this, SLOT(slotCategorizedSortingChanged()));
    connect(view, SIGNAL(sortingChanged(DolphinView::Sorting)),
            this, SLOT(slotSortingChanged(DolphinView::Sorting)));
    connect(view, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(slotSortOrderChanged(Qt::SortOrder)));
    connect(view, SIGNAL(additionalInfoChanged(KFileItemDelegate::AdditionalInformation)),
            this, SLOT(slotAdditionalInfoChanged(KFileItemDelegate::AdditionalInformation)));
    connect(view, SIGNAL(selectionChanged(KFileItemList)),
            this, SLOT(slotSelectionChanged(KFileItemList)));
    connect(view, SIGNAL(requestItemInfo(KUrl)),
            this, SLOT(slotRequestItemInfo(KUrl)));
    connect(view, SIGNAL(activated()),
            this, SLOT(toggleActiveView()));

    const KUrlNavigator* navigator = container->urlNavigator();
    connect(navigator, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(changeUrl(const KUrl&)));
    connect(navigator, SIGNAL(historyChanged()),
            this, SLOT(slotHistoryChanged()));
}

void DolphinMainWindow::updateSplitAction(bool isSplit)
{
    QAction* splitAction = actionCollection()->action("split_view");
    if (isSplit) {
        splitAction->setText(i18n("Join"));
        splitAction->setIcon(KIcon("fileview-join"));
    } else {
        splitAction->setText(i18n("Split"));
        splitAction->setIcon(KIcon("fileview-split"));
    }
}

DolphinMainWindow::UndoUiInterface::UndoUiInterface(DolphinMainWindow* mainWin) :
    KonqUndoManager::UiInterface(mainWin),
    m_mainWin(mainWin)
{
    Q_ASSERT(m_mainWin != 0);
}

DolphinMainWindow::UndoUiInterface::~UndoUiInterface()
{
}

void DolphinMainWindow::UndoUiInterface::jobError(KIO::Job* job)
{
    DolphinStatusBar* statusBar = m_mainWin->activeViewContainer()->statusBar();
    statusBar->setMessage(job->errorString(), DolphinStatusBar::Error);
}

#include "dolphinmainwindow.moc"
