/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   Cvetoslav Ludmiloff                                                   *
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

#include "dolphincontextmenu.h"

#include "dolphinmainwindow.h"
#include "dolphinsettings.h"
#include "dolphinview.h"
#include "dolphinviewcontainer.h"

#include <kactioncollection.h>
#include <kfileplacesmodel.h>
#include <kdesktopfile.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <knewmenu.h>
#include <konqmimedata.h>
#include <konq_operations.h>
#include <klocale.h>
#include <kpropertiesdialog.h>
#include <krun.h>
#include <kstandardaction.h>
#include <kstandarddirs.h>

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtCore/QDir>
#include <Qt3Support/Q3ValueList>

DolphinContextMenu::DolphinContextMenu(DolphinMainWindow* parent,
                                       KFileItem* fileInfo,
                                       const KUrl& baseUrl) :
    m_mainWindow(parent),
    m_fileInfo(fileInfo),
    m_baseUrl(baseUrl),
    m_context(NoContext)
{
    // The context menu either accesses the URLs of the selected items
    // or the items itself. To increase the performance both lists are cached.
    DolphinView* view = m_mainWindow->activeViewContainer()->view();
    m_selectedUrls = view->selectedUrls();
    m_selectedItems = view->selectedItems();
}

DolphinContextMenu::~DolphinContextMenu()
{
}

void DolphinContextMenu::open()
{
    // get the context information
    if (m_baseUrl.protocol() == "trash") {
        m_context |= TrashContext;
    }

    if (m_fileInfo != 0) {
        m_context |= ItemContext;
        // TODO: handle other use cases like devices + desktop files
    }

    // open the corresponding popup for the context
    if (m_context & TrashContext) {
        if (m_context & ItemContext) {
            openTrashItemContextMenu();
        } else {
            openTrashContextMenu();
        }
    } else if (m_context & ItemContext) {
        openItemContextMenu();
    } else {
        Q_ASSERT(m_context == NoContext);
        openViewportContextMenu();
    }
}


void DolphinContextMenu::openTrashContextMenu()
{
    Q_ASSERT(m_context & TrashContext);

    KMenu* popup = new KMenu(m_mainWindow);

    QAction* emptyTrashAction = new QAction(KIcon("emptytrash"), i18n("Empty Trash"), popup);
    KConfig trashConfig("trashrc", KConfig::OnlyLocal);
    emptyTrashAction->setEnabled(!trashConfig.group("Status").readEntry("Empty", true));
    popup->addAction(emptyTrashAction);

    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    popup->addAction(propertiesAction);

    if (popup->exec(QCursor::pos()) == emptyTrashAction) {
        const QString text(i18n("Do you really want to empty the Trash? All items will get deleted."));
        const bool del = KMessageBox::warningContinueCancel(m_mainWindow,
                                                            text,
                                                            QString(),
                                                            KGuiItem(i18n("Empty Trash"), KIcon("user-trash"))
                                                           ) == KMessageBox::Continue;
        if (del) {
            KonqOperations::emptyTrash(m_mainWindow);
        }
    }

    popup->deleteLater();
}

void DolphinContextMenu::openTrashItemContextMenu()
{
    Q_ASSERT(m_context & TrashContext);
    Q_ASSERT(m_context & ItemContext);

    KMenu* popup = new KMenu(m_mainWindow);

    QAction* restoreAction = new QAction(i18n("Restore"), m_mainWindow);
    popup->addAction(restoreAction);

    QAction* deleteAction = m_mainWindow->actionCollection()->action("delete");
    popup->addAction(deleteAction);

    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    popup->addAction(propertiesAction);

    if (popup->exec(QCursor::pos()) == restoreAction) {
        KonqOperations::restoreTrashedItems(m_selectedUrls, m_mainWindow);
    }

    popup->deleteLater();
}

void DolphinContextMenu::openItemContextMenu()
{
    Q_ASSERT(m_fileInfo != 0);

    KMenu* popup = new KMenu(m_mainWindow);
    insertDefaultItemActions(popup);

    popup->addSeparator();

    // insert 'Bookmark This Folder' entry if exactly one item is selected
    QAction* bookmarkAction = 0;
    if (m_fileInfo->isDir() && (m_selectedUrls.count() == 1)) {
        bookmarkAction = popup->addAction(KIcon("bookmark-folder"), i18n("Bookmark Folder..."));
    }

    // Insert 'Open With...' sub menu
    QVector<KService::Ptr> openWithVector;
    const QList<QAction*> openWithActions = insertOpenWithItems(popup, openWithVector);

    // Insert 'Actions' sub menu
    QVector<KDesktopFileActions::Service> actionsVector;
    const QList<QAction*> serviceActions = insertActionItems(popup, actionsVector);
    popup->addSeparator();

    // insert 'Properties...' entry
    QAction* propertiesAction = m_mainWindow->actionCollection()->action("properties");
    popup->addAction(propertiesAction);

    QAction* activatedAction = popup->exec(QCursor::pos());

    if ((bookmarkAction != 0) && (activatedAction == bookmarkAction)) {
        const KUrl selectedUrl(m_fileInfo->url());
        if (selectedUrl.isValid()) {
            DolphinSettings::instance().placesModel()->addPlace(selectedUrl.fileName(),
                                                                selectedUrl);
        }
    } else if (serviceActions.contains(activatedAction)) {
        // one of the 'Actions' items has been selected
        int id = serviceActions.indexOf(activatedAction);
        KDesktopFileActions::executeService(m_selectedUrls, actionsVector[id]);
    } else if (openWithActions.contains(activatedAction)) {
        // one of the 'Open With' items has been selected
        if (openWithActions.last() == activatedAction) {
            // the item 'Other...' has been selected
            KRun::displayOpenWithDialog(m_selectedUrls, m_mainWindow);
        } else {
            int id = openWithActions.indexOf(activatedAction);
            KService::Ptr servicePtr = openWithVector[id];
            KRun::run(*servicePtr, m_selectedUrls, m_mainWindow);
        }
    }

    openWithVector.clear();
    actionsVector.clear();
    popup->deleteLater();
}

void DolphinContextMenu::openViewportContextMenu()
{
    Q_ASSERT(m_fileInfo == 0);
    KMenu* popup = new KMenu(m_mainWindow);

    // setup 'Create New' menu
    KNewMenu* newMenu = m_mainWindow->newMenu();
    newMenu->slotCheckUpToDate();
    newMenu->setPopupFiles(m_baseUrl);
    popup->addMenu(newMenu->menu());
    popup->addSeparator();

    QAction* pasteAction = m_mainWindow->actionCollection()->action(KStandardAction::stdName(KStandardAction::Paste));
    popup->addAction(pasteAction);

    // setup 'View Mode' menu
    KMenu* viewModeMenu = new KMenu(i18n("View Mode"));

    QAction* iconsMode = m_mainWindow->actionCollection()->action("icons");
    viewModeMenu->addAction(iconsMode);

    QAction* detailsMode = m_mainWindow->actionCollection()->action("details");
    viewModeMenu->addAction(detailsMode);

    QAction* columnsMode = m_mainWindow->actionCollection()->action("columns");
    viewModeMenu->addAction(columnsMode);

    QAction* previewsMode = m_mainWindow->actionCollection()->action("previews");
    viewModeMenu->addAction(previewsMode);

    popup->addMenu(viewModeMenu);

    QAction* toggleViewsAction = 0;
    if (m_mainWindow->isSplit()) {
        toggleViewsAction = popup->addAction(i18n("Toggle Views"));
    }

    popup->addSeparator();

    QAction* bookmarkAction = popup->addAction(KIcon("bookmark-folder"), i18n("Bookmark This Folder..."));
    popup->addSeparator();

    QAction* propertiesAction = popup->addAction(i18n("Properties"));

    QAction* action = popup->exec(QCursor::pos());
    if (action == propertiesAction) {
        const KUrl& url = m_mainWindow->activeViewContainer()->url();
        KPropertiesDialog dialog(url);
        dialog.exec();
    } else if (action == bookmarkAction) {
        const KUrl& url = m_mainWindow->activeViewContainer()->url();
        if (url.isValid()) {
            DolphinSettings::instance().placesModel()->addPlace(url.fileName(), url);
        }
    } else if ((toggleViewsAction != 0) && (action == toggleViewsAction)) {
        m_mainWindow->toggleViews();
    }

    popup->deleteLater();
}

void DolphinContextMenu::insertDefaultItemActions(KMenu* popup)
{
    Q_ASSERT(popup != 0);
    const KActionCollection* collection = m_mainWindow->actionCollection();

    // insert 'Cut', 'Copy' and 'Paste'
    QAction* cutAction = collection->action(KStandardAction::stdName(KStandardAction::Cut));
    QAction* copyAction  = collection->action(KStandardAction::stdName(KStandardAction::Copy));
    QAction* pasteAction = collection->action(KStandardAction::stdName(KStandardAction::Paste));

    popup->addAction(cutAction);
    popup->addAction(copyAction);
    popup->addAction(pasteAction);
    popup->addSeparator();

    // insert 'Rename'
    QAction* renameAction = collection->action("rename");
    popup->addAction(renameAction);

    // insert 'Move to Trash' and (optionally) 'Delete'
    const KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::NoGlobals);
    const KConfigGroup kdeConfig(globalConfig, "KDE");
    bool showDeleteCommand = kdeConfig.readEntry("ShowDeleteCommand", false);
    const KUrl& url = m_mainWindow->activeViewContainer()->url();
    if (url.isLocalFile()) {
        QAction* moveToTrashAction = collection->action("move_to_trash");
        popup->addAction(moveToTrashAction);
    } else {
        showDeleteCommand = true;
    }

    if (showDeleteCommand) {
        QAction* deleteAction = collection->action("delete");
        popup->addAction(deleteAction);
    }
}

QList<QAction*> DolphinContextMenu::insertOpenWithItems(KMenu* popup,
        QVector<KService::Ptr>& openWithVector)
{
    // Parts of the following code have been taken
    // from the class KonqOperations located in
    // libqonq/konq_operations.h of Konqueror.
    // (Copyright (C) 2000  David Faure <faure@kde.org>)

    // Prepare 'Open With' sub menu. Usually a sub menu is created, where all applications
    // are listed which are registered to open the item. As last entry "Other..." will be
    // attached which allows to select a custom application. If no applications are registered
    // no sub menu is created at all, only "Open With..." will be offered.
    bool insertOpenWithItems = true;
    const QString contextMimeType(m_fileInfo->mimetype());

    QListIterator<KFileItem*> mimeIt(m_selectedItems);
    while (insertOpenWithItems && mimeIt.hasNext()) {
        KFileItem* item = mimeIt.next();
        insertOpenWithItems = (contextMimeType == item->mimetype());
    }

    QList<QAction*> openWithActions;
    if (insertOpenWithItems) {
        // fill the 'Open with' sub menu with application types
        const KMimeType::Ptr mimePtr = KMimeType::findByUrl(m_fileInfo->url());
        KService::List offers = KMimeTypeTrader::self()->query(mimePtr->name(),
                                "Application",
                                "Type == 'Application'");
        if (offers.count() > 0) {
            KService::List::Iterator it;
            KMenu* openWithMenu = new KMenu(i18n("Open With"));
            for (it = offers.begin(); it != offers.end(); ++it) {
                // The offer list from the KTrader returns duplicate
                // application entries. Although this seems to be a configuration
                // problem outside the scope of Dolphin, duplicated entries just
                // will be skipped here.
                const QString appName((*it)->name());
                if (!containsEntry(openWithMenu, appName)) {
                    const KIcon icon((*it)->icon());
                    QAction* action = openWithMenu->addAction(icon, appName);
                    openWithVector.append(*it);
                    openWithActions << action;
                }
            }

            openWithMenu->addSeparator();
            QAction* action = openWithMenu->addAction(i18n("&Other..."));

            openWithActions << action;
            popup->addMenu(openWithMenu);
        } else {
            // No applications are registered, hence just offer
            // a "Open With..." item instead of a sub menu containing
            // only one entry.
            QAction* action = popup->addAction(i18n("Open With..."));
            openWithActions << action;
        }
    } else {
        // At least one of the selected items has a different MIME type. In this case
        // just show a disabled "Open With..." entry.
        QAction* action = popup->addAction(i18n("Open With..."));
        action->setEnabled(false);
    }

    return openWithActions;
}

QList<QAction*> DolphinContextMenu::insertActionItems(KMenu* popup,
                                                      QVector<KDesktopFileActions::Service>& actionsVector)
{
    // Parts of the following code have been taken
    // from the class KonqOperations located in
    // libqonq/konq_operations.h of Konqueror.
    // (Copyright (C) 2000  David Faure <faure@kde.org>)

    KMenu* actionsMenu = new KMenu(i18n("Actions"));

    QList<QAction*> serviceActions;

    QStringList dirs = KGlobal::dirs()->findDirs("data", "dolphin/servicemenus/");

    KMenu* menu = 0;
    for (QStringList::ConstIterator dirIt = dirs.begin(); dirIt != dirs.end(); ++dirIt) {
        QDir dir(*dirIt);
        QStringList filters;
        filters << "*.desktop";
        dir.setNameFilters(filters);
        QStringList entries = dir.entryList(QDir::Files);

        for (QStringList::ConstIterator entryIt = entries.begin(); entryIt != entries.end(); ++entryIt) {
            KConfigGroup cfg(KSharedConfig::openConfig(*dirIt + *entryIt, KConfig::OnlyLocal), "Desktop Entry");
            if ((cfg.hasKey("Actions") || cfg.hasKey("X-KDE-GetActionMenu")) && cfg.hasKey("ServiceTypes")) {
                //const QStringList types = cfg.readListEntry("ServiceTypes");
                QStringList types;
                types = cfg.readEntry("ServiceTypes", types);
                for (QStringList::ConstIterator it = types.begin(); it != types.end(); ++it) {
                    // check whether the mime type is equal or whether the
                    // mimegroup (e. g. image/*) is supported

                    bool insert = false;
                    if ((*it) == "all/allfiles") {
                        // The service type is valid for all files, but not for directories.
                        // Check whether the selected items only consist of files...
                        QListIterator<KFileItem*> mimeIt(m_selectedItems);
                        insert = true;
                        while (insert && mimeIt.hasNext()) {
                            KFileItem* item = mimeIt.next();
                            insert = !item->isDir();
                        }
                    }

                    if (!insert) {
                        // Check whether the MIME types of all selected files match
                        // to the mimetype of the service action. As soon as one MIME
                        // type does not match, no service menu is shown at all.
                        QListIterator<KFileItem*> mimeIt(m_selectedItems);
                        insert = true;
                        while (insert && mimeIt.hasNext()) {
                            KFileItem* item = mimeIt.next();
                            const QString mimeType(item->mimetype());
                            const QString mimeGroup(mimeType.left(mimeType.indexOf('/')));

                            insert  = (*it == mimeType) ||
                                      ((*it).right(1) == "*") &&
                                      ((*it).left((*it).indexOf('/')) == mimeGroup);
                        }
                    }

                    if (insert) {
                        menu = actionsMenu;

                        const QString submenuName = cfg.readEntry("X-KDE-Submenu");
                        if (!submenuName.isEmpty()) {
                            menu = new KMenu(submenuName);
                            actionsMenu->addMenu(menu);
                        }

                        Q3ValueList<KDesktopFileActions::Service> userServices =
                            KDesktopFileActions::userDefinedServices(*dirIt + *entryIt, true);

                        Q3ValueList<KDesktopFileActions::Service>::Iterator serviceIt;
                        for (serviceIt = userServices.begin(); serviceIt != userServices.end(); ++serviceIt) {
                            KDesktopFileActions::Service service = (*serviceIt);
                            if (!service.m_strIcon.isEmpty()) {
                                QAction* action = menu->addAction(KIcon(service.m_strIcon),
                                                                  service.m_strName);
                                serviceActions << action;
                            } else {
                                QAction *action = menu->addAction(service.m_strName);
                                serviceActions << action;
                            }
                            actionsVector.append(service);
                        }
                    }
                }
            }
        }
    }

    const int itemsCount = actionsMenu->actions().count();
    if (itemsCount == 0) {
        // no actions are available at all, hence show the "Actions"
        // submenu disabled
        actionsMenu->setEnabled(false);
    }

    if (itemsCount == 1) {
        // Exactly one item is available. Instead of showing a sub menu with
        // only one item, show the item directly in the root menu.
        if (menu == actionsMenu) {
            // The item is an action, hence show the action in the root menu.
            const QList<QAction*> actions = actionsMenu->actions();
            Q_ASSERT(actions.count() == 1);

            const QString text = actions[0]->text();
            const QIcon icon = actions[0]->icon();
            if (icon.isNull()) {
                QAction* action = popup->addAction(text);
                serviceActions.clear();
                serviceActions << action;
            } else {
                QAction* action = popup->addAction(icon, text);
                serviceActions.clear();
                serviceActions << action;
            }
        } else {
            // The item is a sub menu, hence show the sub menu in the root menu.
            popup->addMenu(menu);
        }
        actionsMenu->deleteLater();
        actionsMenu = 0;
    } else {
        popup->addMenu(actionsMenu);
    }

    return serviceActions;
}

bool DolphinContextMenu::containsEntry(const KMenu* menu,
                                       const QString& entryName) const
{
    Q_ASSERT(menu != 0);

    const QList<QAction*> list = menu->actions();
    const uint count = list.count();
    for (uint i = 0; i < count; ++i) {
        const QAction* action = list.at(i);
        if (action->text() == entryName) {
            return true;
        }
    }

    return false;
}

#include "dolphincontextmenu.moc"
