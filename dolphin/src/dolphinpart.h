/* This file is part of the KDE project
   Copyright (c) 2007 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DOLPHINPART_H
#define DOLPHINPART_H

#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kparts/fileinfoextension.h>

#include <QItemSelectionModel>

class KNewFileMenu;
class DolphinViewActionHandler;
class QActionGroup;
class KAction;
class KFileItemList;
class KFileItem;
class DolphinPartBrowserExtension;
class DolphinSortFilterProxyModel;
class DolphinRemoteEncoding;
class DolphinModel;
class KDirLister;
class DolphinView;
class KAboutData;

class DolphinPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
    // Used by konqueror. Technically it means "we want undo enabled if
    // there are things in the undo history and the current part is a dolphin part".
    // Even though it's konqueror doing the undo...
    Q_PROPERTY( bool supportsUndo READ supportsUndo )

    Q_PROPERTY( QString currentViewMode READ currentViewMode WRITE setCurrentViewMode )

    // Used by konqueror when typing something like /home/dfaure/*.diff in the location bar
    Q_PROPERTY( QString nameFilter READ nameFilter WRITE setNameFilter )

    // Used by konqueror to implement the --select command-line option
    Q_PROPERTY( KUrl::List filesToSelect READ filesToSelect WRITE setFilesToSelect )

public:
    explicit DolphinPart(QWidget* parentWidget, QObject* parent, const QVariantList& args);
    ~DolphinPart();

    static KAboutData* createAboutData();

    /**
     * Standard KParts::ReadOnlyPart openUrl method.
     * Called by Konqueror to view a directory in DolphinPart.
     */
    virtual bool openUrl(const KUrl& url);

    /// see the supportsUndo property
    bool supportsUndo() const { return true; }

    /**
     * Used by konqueror for setting the view mode
     * @param viewModeName internal name for the view mode, like "icons"
     * Those names come from the Actions line in dolphinpart.desktop,
     * and have to match the name of the KActions.
     */
    void setCurrentViewMode(const QString& viewModeName);

    /**
     * Used by konqueror for displaying the current view mode.
     * @see setCurrentViewMode
     */
    QString currentViewMode() const;

    /// Returns the view owned by this part; used by DolphinPartBrowserExtension
    DolphinView* view() { return m_view; }

    /**
     * Sets a name filter, like *.diff
     */
    void setNameFilter(const QString& nameFilter);

    /**
     * Returns the current name filter. Used by konqueror to show it in the URL.
     */
    QString nameFilter() const { return m_nameFilter; }

protected:
    /**
     * We reimplement openUrl so no need to implement openFile.
     */
    virtual bool openFile() { return true; }

Q_SIGNALS:
    /**
     * Emitted when the view mode changes. Used by konqueror.
     */
    void viewModeChanged();


    /**
     * Emitted whenever the current URL is about to be changed.
     */
    void aboutToOpenURL();

private Q_SLOTS:
    void slotMessage(const QString& msg);
    void slotErrorMessage(const QString& msg);
    /**
     * Shows the information for the item \a item inside the statusbar. If the
     * item is null, the default statusbar information is shown.
     */
    void slotRequestItemInfo(const KFileItem& item);
    /**
     * Handles clicking on an item
     */
    void slotItemActivated(const KFileItem& item);
    /**
     * Creates a new window showing the content of \a url.
     */
    void createNewWindow(const KUrl& url);
    /**
     * Opens the context menu on the current mouse position.
     * @pos           Position in screen coordinates.
     * @item          File item context. If item is null, the context menu
     *                should be applied to \a url.
     * @url           URL which contains \a item.
     * @customActions Actions that should be added to the context menu,
     *                if the file item is null.
     */
    void slotOpenContextMenu(const QPoint& pos,
                             const KFileItem& item,
                             const KUrl& url,
                             const QList<QAction*>& customActions);

    /**
     * Informs the host that we are opening \a url (e.g. after a redirection
     * coming from KDirLister).
     * Testcase 1: fish://localhost
     * Testcase 2: showing a directory that is being renamed by another window (#180156)
     */
    void slotDirectoryRedirection(const KUrl& oldUrl, const KUrl& newUrl);

    /**
     * Updates the state of the 'Edit' menu actions and emits
     * the signal selectionChanged().
     */
    void slotSelectionChanged(const KFileItemList& selection);

    /**
     * Updates the text of the paste action dependent from
     * the number of items which are in the clipboard.
     */
    void updatePasteAction();

    /**
     * Connected to all "Go" menu actions provided by DolphinPart
     */
    void slotGoTriggered(QAction* action);

    /**
     * Connected to the "editMimeType" action
     */
    void slotEditMimeType();

    /**
     * Connected to the "select_items_matching" action.
     * Opens a dialog which permits to select all items matching a pattern like "*.jpg".
     */
    void slotSelectItemsMatchingPattern();

    /**
     * Connected to the "unselect_items_matching" action.
     * Opens a dialog which permits to unselect all items matching a pattern like "*.jpg".
     */
    void slotUnselectItemsMatchingPattern();

    /**
     * Open a terminal window, starting with the current directory.
     */
    void slotOpenTerminal();

    /**
     * Open KFind with the current path.
     */
    void slotFindFile();

    /**
     * Updates the 'Create New...' sub menu, just before it's shown.
     */
    void updateNewMenu();

    /**
     * Updates the number of items (= number of files + number of
     * directories) in the statusbar. If files are selected, the number
     * of selected files and the sum of the filesize is shown.
     */
    void updateStatusBar();

   /**
    * Notify container of folder loading progress.
    */
    void updateProgress(int percent);

    void createDirectory();

    /**
     * Called by konqueror --select
     */
    void setFilesToSelect(const KUrl::List& files);
    KUrl::List filesToSelect() const { return KUrl::List(); } // silence moc

private:
    void createActions();
    void createGoAction(const char* name, const char* iconName,
                        const QString& text, const QString& url,
                        QActionGroup* actionGroup);

    void openSelectionDialog(const QString& title, const QString& text,
                             bool selectItems);

private:
    DolphinView* m_view;
    DolphinViewActionHandler* m_actionHandler;
    DolphinRemoteEncoding* m_remoteEncoding;
    DolphinPartBrowserExtension* m_extension;
    KNewFileMenu* m_newFileMenu;
    QString m_nameFilter;
    Q_DISABLE_COPY(DolphinPart)
};

class DolphinPartBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
public:
    DolphinPartBrowserExtension( DolphinPart* part )
        : KParts::BrowserExtension( part ), m_part(part) {}

    virtual void restoreState(QDataStream &stream);
    virtual void saveState(QDataStream &stream);

public Q_SLOTS:
    void cut();
    void copy();
    void paste();
    void pasteTo(const KUrl&);
    void reparseConfiguration();

private:
    DolphinPart* m_part;
};


class DolphinPartFileInfoExtension : public KParts::FileInfoExtension
{
    Q_OBJECT

public:
    DolphinPartFileInfoExtension(DolphinPart* part);

    virtual QueryModes supportedQueryModes() const;
    virtual bool hasSelection() const;

    virtual KFileItemList queryFor(QueryMode mode) const;
protected:
    DolphinPart* part() const;
};

#endif /* DOLPHINPART_H */
