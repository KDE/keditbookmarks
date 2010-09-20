/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef DOLPHINCONTEXTMENU_H
#define DOLPHINCONTEXTMENU_H

#include <kdesktopfileactions.h>
#include <kfileitem.h>
#include <kservice.h>
#include <kurl.h>
#include <konq_copytomenu.h>
#include <kmodifierkeyinfo.h>

#include <QtCore/QObject>

#include <QtCore/QVector>

#include <QScopedPointer>

class KMenu;
class KFileItem;
class QAction;
class DolphinMainWindow;
class KFileItemActions;
class KFileItemListProperties;

/**
 * @brief Represents the context menu which appears when doing a right
 *        click on an item or the viewport of the file manager.
 *
 * Beside static menu entries (e. g. 'Paste' or 'Properties') two
 * dynamic sub menus are shown when opening a context menu above
 * an item:
 * - 'Open With': Contains all applications which are registered to
 *                open items of the given MIME type.
 * - 'Actions':   Contains all actions which can be applied to the
 *                given item.
 */
class DolphinContextMenu : public QObject
{
    Q_OBJECT

public:
    /**
     * @parent        Pointer to the main window the context menu
     *                belongs to.
     * @fileInfo      Pointer to the file item the context menu
     *                is applied. If 0 is passed, the context menu
     *                is above the viewport.
     * @baseUrl       Base URL of the viewport where the context menu
     *                should be opened.
     */
    DolphinContextMenu(DolphinMainWindow* parent,
                       const KFileItem& fileInfo,
                       const KUrl& baseUrl);

    virtual ~DolphinContextMenu();

    void setCustomActions(const QList<QAction*>& actions);

    /** Opens the context menu model. */
    void open();

    /**
     * TODO: This method is a workaround for a X11-issue in combination
     * with KModifierKeyInfo: When constructing KModifierKeyInfo in the
     * constructor of the context menu, the user interface might freeze.
     * To bypass this, the KModifierKeyInfo is constructed in DolphinMainWindow
     * directly after starting the application. Remove this method, if
     * the X11-issue got fixed (contact the maintainer of KModifierKeyInfo for
     * more details).
     */
    static void initializeModifierKeyInfo();

private slots:
    /**
     * Is invoked if a key modifier has been pressed and updates the context
     * menu to show the 'Delete' action instead of the 'Move To Trash' action
     * if the shift-key has been pressed.
     */
    void slotKeyModifierPressed(Qt::Key key, bool pressed);

    /**
     * Triggers the 'Delete'-action if the shift-key has been pressed, otherwise
     * the 'Move to Trash'-action gets triggered.
     */
    void slotRemoveActionTriggered();

private:
    void openTrashContextMenu();
    void openTrashItemContextMenu();
    void openItemContextMenu();
    void openViewportContextMenu();

    void insertDefaultItemActions();

    /**
     * Adds the "Show menubar" action to the menu if the
     * menubar is hidden.
     */
    void addShowMenubarAction();

    /**
     * Returns a name for adding the URL \a url to the Places panel.
     */
    QString placesName(const KUrl& url) const;

    bool placeExists(const KUrl& url) const;

    QAction* createPasteAction();

    KFileItemListProperties& capabilities();
    void addServiceActions(KFileItemActions& fileItemActions);
    void addVersionControlActions();
    void addCustomActions();

    /**
     * Updates m_removeAction to represent the 'Delete'-action if the shift-key
     * has been pressed. Otherwise it represents the 'Move to Trash'-action.
     */
    void updateRemoveAction();

private:
    struct Entry
    {
        int type;
        QString name;
        QString filePath;     // empty for separator
        QString templatePath; // same as filePath for template
        QString icon;
        QString comment;
    };

    enum ContextType
    {
        NoContext = 0,
        ItemContext = 1,
        TrashContext = 2
    };

    DolphinMainWindow* m_mainWindow;
    KFileItemListProperties* m_capabilities;
    KFileItem m_fileInfo;
    KUrl m_baseUrl;
    KFileItemList m_selectedItems;
    KUrl::List m_selectedUrls;
    int m_context;
    KonqCopyToMenu m_copyToMenu;
    QList<QAction*> m_customActions;
    QScopedPointer<KMenu> m_popup;

    bool m_shiftPressed;
    QAction* m_removeAction; // Action that represents either 'Move To Trash' or 'Delete'

    static KModifierKeyInfo* m_keyInfo;
};

#endif
