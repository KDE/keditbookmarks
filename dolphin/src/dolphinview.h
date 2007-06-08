/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Gregor Kališnik <gregor@podnapisi.net>          *
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


#ifndef DOLPHINVIEW_H
#define DOLPHINVIEW_H

#include <kparts/part.h>
#include <kfileitem.h>
#include <kfileitemdelegate.h>
#include <kio/job.h>

#include <QBoxLayout>
#include <QKeyEvent>
#include <QLinkedList>
#include <QListView>
#include <QWidget>

class DolphinController;
class KDirLister;
class KFileItemDelegate;
class KUrl;
class KDirModel;
class DolphinColumnView;
class DolphinDetailsView;
class DolphinIconsView;
class DolphinMainWindow;
class DolphinSortFilterProxyModel;
class QModelIndex;
class ViewProperties;

/**
 * @short Represents a view for the directory content.
 *
 * View modes for icons, details and columns are supported. It's
 * possible to adjust:
 * - sort order
 * - sort type
 * - show hidden files
 * - show previews
 *
 * @see DolphinIconsView
 * @see DolphinDetailsView
 * @see DolphinColumnView
 */
class DolphinView : public QWidget
{
    Q_OBJECT

public:
    /**
        * Defines the view mode for a directory. The view mode
        * can be defined when constructing a DolphinView. The
        * view mode is automatically updated if the directory itself
        * defines a view mode (see class ViewProperties for details).
        */
    enum Mode
    {
        /**
         * The directory items are shown as icons including an
         * icon name. */
        IconsView = 0,

        /**
         * The icon, the name and at least the size of the directory
         * items are shown in a table. It is possible to add columns
         * for date, group and permissions.
         */
        DetailsView = 1,

        /**
         * Each folder is shown in a separate column.
         */
        ColumnView = 2,
        MaxModeEnum = ColumnView
    };

    /** Defines the sort order for the items of a directory. */
    enum Sorting
    {
        SortByName = 0,
        SortBySize,
        SortByDate,
        SortByPermissions,
        SortByOwner,
        SortByGroup,
        SortByType,
        MaxSortEnum = SortByType
    };

    /**
     * @param parent          Parent widget of the view.
     * @param url             Specifies the content which should be shown.
     * @param dirLister       Used directory lister. The lister is not owned
     *                        by the view and won't get deleted.
     * @param dirModel        Used directory model. The model is not owned
     *                        by the view and won't get deleted.
     * @param proxyModel      Used proxy model which specifies the sorting. The
     *                        model is not owned by the view and won't get
     *                        deleted.
     * @param mode            Used display mode (IconsView, DetailsView or ColumnsView).
     * @param showHiddenFiles If true, hidden files will be shown in the view.
     */
    DolphinView(QWidget* parent,
                const KUrl& url,
                KDirLister* dirLister,
                KDirModel* dirModel,
                DolphinSortFilterProxyModel* proxyModel,
                Mode mode);

    virtual ~DolphinView();

    /**
     * Returns the current active URL, where all actions are applied.
     * The URL navigator is synchronized with this URL.
     */
    const KUrl& url() const;

    /**
     * Returns the root URL of the view, which is defined as the first
     * visible path of DolphinView::url(). Usually the root URL is
     * equal to DolphinView::url(), but in the case of the column view
     * when 2 columns are shown, the root URL might be:
     * /home/peter/Documents
     * and DolphinView::url() might return
     * /home/peter/Documents/Music/
     */
    KUrl rootUrl() const;

    /**
     * If \a active is true, the view will marked as active. The active
     * view is defined as view where all actions are applied to.
     */
    void setActive(bool active);
    bool isActive() const;

    /**
     * Changes the view mode for the current directory to \a mode.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * changed view mode will be be stored automatically.
     */
    void setMode(Mode mode);
    Mode mode() const;

    /**
     * Turns on the file preview for the all files of the current directory,
     * if \a show is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * preview setting will be be stored automatically.
     */
    void setShowPreview(bool show);
    bool showPreview() const;

    /**
     * Shows all hidden files of the current directory,
     * if \a show is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * show hidden file setting will be be stored automatically.
     */
    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const;

    /**
     * Summarizes all sorted items by their category \a categorized
     * is true.
     * If the view properties should be remembered for each directory
     * (GeneralSettings::globalViewProps() returns false), then the
     * categorized sorting setting will be be stored automatically.
     */
    void setCategorizedSorting(bool categorized);
    bool categorizedSorting() const;

    /**
     * Returns true, if the categorized sorting is supported by the current
     * used mode (see DolphinView::setMode()). Currently only DolphinView::IconsView
     * supports categorizations. To check whether the categorized
     * sorting is set, use DolphinView::categorizedSorting().
     */
    bool supportsCategorizedSorting() const;

    /**
     * Selects all items.
     * @see DolphinView::selectedItems()
     */
    void selectAll();

    /**
     * Inverts the current selection: selected items get unselected,
     * unselected items get selected.
     * @see DolphinView::selectedItems()
     */
    void invertSelection();

    /** Returns true, if at least one item is selected. */
    bool hasSelection() const;

    void clearSelection();

    /**
     * Returns the selected items. The list is empty if no item has been
     * selected.
     * @see DolphinView::selectedUrls()
     */
    KFileItemList selectedItems() const;

    /**
     * Returns a list of URLs for all selected items. An empty list
     * is returned, if no item is selected.
     * @see DolphinView::selectedItems()
     */
    KUrl::List selectedUrls() const;

    /**
     * Returns the file item for the given model index \a index.
     */
    KFileItem* fileItem(const QModelIndex index) const;

    /**
     * Returns the x-position of the view content.
     * The content of the view might be larger than the visible area
     * and hence a scrolling must be done.
     */
    int contentsX() const;

    /**
     * Returns the y-position of the view content.
     * The content of the view might be larger than the visible area
     * and hence a scrolling must be done.
     */
    int contentsY() const;

    /** Increases the size of the current set view mode. */
    void zoomIn();

    /** Decreases the size of the current set view mode. */
    void zoomOut();

    /**
     * Returns true, if zooming in is possible. If false is returned,
     * the minimal zoom size is possible.
     */
    bool isZoomInPossible() const;

    /**
     * Returns true, if zooming out is possible. If false is returned,
     * the maximum zoom size is possible.
     */
    bool isZoomOutPossible() const;

    /** Sets the sort order of the items inside a directory (see DolphinView::Sorting). */
    void setSorting(Sorting sorting);

    /** Returns the sort order of the items inside a directory (see DolphinView::Sorting). */
    Sorting sorting() const;

    /** Sets the sort order (Qt::Ascending or Qt::Descending) for the items. */
    void setSortOrder(Qt::SortOrder order);

    /** Returns the current used sort order (Qt::Ascending or Qt::Descending). */
    Qt::SortOrder sortOrder() const;

    /** Sets the additional information which should be shown for the items. */
    void setAdditionalInfo(KFileItemDelegate::AdditionalInformation info);

    /** Returns the additional information which should be shown for the items. */
    KFileItemDelegate::AdditionalInformation additionalInfo() const;

    /** Reloads the current directory. */
    void reload();

    /**
     * Refreshs the view to get synchronized with the (updated) Dolphin settings.
     * This method only needs to get invoked if the view settings for the Icons View,
     * Details View or Columns View have been changed.
     */
    void refresh();

public slots:
    /**
     * Changes the directory to \a url. If the current directory is equal to
     * \a url, nothing will be done (use DolphinView::reload() instead).
     */
    void setUrl(const KUrl& url);

    /**
     * Request of a selection change. The view will do its best to accommodate
     * the request, but it is not guaranteed that all items in \a selection
     * will actually get selected. The view will e.g. not select items which
     * are not in the currently displayed folder.
     */
    void changeSelection(const KFileItemList& selection);

signals:
    /**
     * Is emitted if the view has been activated by e. g. a mouse click.
     */
    void activated();

    /** Is emitted if URL of the view has been changed to \a url. */
    void urlChanged(const KUrl& url);

    /**
     * Is emitted if the view mode (IconsView, DetailsView,
     * PreviewsView) has been changed.
     */
    void modeChanged();

    /** Is emitted if the 'show preview' property has been changed. */
    void showPreviewChanged();

    /** Is emitted if the 'show hidden files' property has been changed. */
    void showHiddenFilesChanged();

    /** Is emitted if the 'categorized sorting' property has been changed. */
    void categorizedSortingChanged();

    /** Is emitted if the sorting by name, size or date has been changed. */
    void sortingChanged(DolphinView::Sorting sorting);

    /** Is emitted if the sort order (ascending or descending) has been changed. */
    void sortOrderChanged(Qt::SortOrder order);

    /** Is emitted if the additional information for an item has been changed. */
    void additionalInfoChanged(KFileItemDelegate::AdditionalInformation info);

    /**
     * Is emitted if information of an item is requested to be shown e. g. in the sidebar.
     * It the URL is empty, no item information request is pending.
     */
    void requestItemInfo(const KUrl& url);

    /** Is emitted if the contents has been moved to \a x, \a y. */
    void contentsMoved(int x, int y);

    /**
     * Is emitted whenever the selection has been changed.
     */
    void selectionChanged(const KFileItemList& selection);

    /**
     * Is emitted if a context menu is requested for the item \a item,
     * which is part of \a url. If the item is 0, the context menu
     * for the URL should be shown.
     */
    void requestContextMenu(KFileItem* item, const KUrl& url);

    /**
     * Is emitted if the URLs \a are dropped to the destination URL
     * \a destination. No operation is done within the DolphinView, the
     * receiver of the signal has to take care about the corresponding
     * operation.
     */
    void urlsDropped(const KUrl::List& urls, const KUrl& destination);

    /**
     * Is emitted if an information message with the content \a msg
     * should be shown.
     */
    void infoMessage(const QString& msg);

    /**
     * Is emitted if an error message with the content \a msg
     * should be shown.
     */
    void errorMessage(const QString& msg);

protected:
    /** @see QWidget::mouseReleaseEvent */
    virtual void mouseReleaseEvent(QMouseEvent* event);

private slots:
    /**
     * Marks the view as active (DolphinView:isActive() will return true)
     * and emits the 'activated' signal if it is not already active.
     */
    void activate();

    /**
     * If the item specified by \a index is a directory, then this
     * directory will be loaded. If the  item is a file, the corresponding
     * application will get started.
     */
    void triggerItem(const QModelIndex& index);

    /**
     * Generates a preview image for each file item in \a items.
     * The current preview settings (maximum size, 'Show Preview' menu)
     * are respected.
     */
    void generatePreviews(const KFileItemList& items);

    /**
     * Replaces the icon of the item \a item by the preview pixmap
     * \a pixmap.
     */
    void showPreview(const KFileItem& item, const QPixmap& pixmap);

    /**
     * Restores the x- and y-position of the contents if the
     * current view is part of the history.
     */
    void restoreContentsPos();

    void emitSelectionChangedSignal();

    /**
     * Opens the context menu on position \a pos. The position
     * is used to check whether the context menu is related to an
     * item or to the viewport.
     */
    void openContextMenu(const QPoint& pos);

    /**
     * Drops the URLs \a urls to the index \a index. \a source
     * indicates the widget where the dragging has been started from.
     */
    void dropUrls(const KUrl::List& urls,
                  const QModelIndex& index,
                  QWidget* source);

    /**
     * Drops the URLs \a urls at the
     * destination \a destination.
     */
    void dropUrls(const KUrl::List& urls,
                  const KUrl& destination);
    /**
     * Updates the view properties of the current URL to the
     * sorting given by \a sorting.
     */
    void updateSorting(DolphinView::Sorting sorting);

    /**
     * Updates the view properties of the current URL to the
     * sort order given by \a order.
     */
    void updateSortOrder(Qt::SortOrder order);

    /**
     * Emits the signal contentsMoved with the current coordinates
     * of the viewport as parameters.
     */
    void emitContentsMoved();

    /** Applies an item effect to all cut items of the clipboard. */
    void updateCutItems();

    /**
     * Updates the status bar to show hover information for the
     * item with the index \a index. If currently other items are selected,
     * no hover information is shown.
     * @see DolphinView::clearHoverInformation()
     */
    void showHoverInformation(const QModelIndex& index);

    /**
     * Clears the hover information shown in the status bar.
     * @see DolphinView::showHoverInformation().
     */
    void clearHoverInformation();

private:
    void startDirLister(const KUrl& url, bool reload = false);

    /**
     * Creates a new view representing the given view mode (DolphinView::mode()).
     * The current view will get deleted.
     */
    void createView();

    /**
     * Selects all items by using the selection flags \a flags. This is a helper
     * method for the slots DolphinView::selectAll() and DolphinView::invertSelection().
     */
    void selectAll(QItemSelectionModel::SelectionFlags flags);

    /**
     * Returns a pointer to the currently used item view, which is either
     * a ListView or a TreeView.
     */
    QAbstractItemView* itemView() const;

    /**
     * Returns true if the index is valid and represents
     * the column KDirModel::Name.
     */
    bool isValidNameIndex(const QModelIndex& index) const;

    /**
     * Returns true, if the item \a item has been cut into
     * the clipboard.
     */
    bool isCutItem(const KFileItem& item) const;

    /** Applies an item effect to all cut items. */
    void applyCutItemEffect();

    /**
     * Returns true, if the ColumnView is activated. As the column view
     * requires some special handling for iterating through directories,
     * this method has been introduced for convenience.
     */
    bool isColumnViewActive() const
    {
        return m_columnView != 0;
    }

private:
    /**
     * Remembers the original pixmap for an item before
     * the cut effect is applied.
     */
    struct CutItem
    {
        KUrl url;
        QPixmap pixmap;
    };

    bool m_active;
    bool m_blockContentsMovedSignal;
    bool m_initializeColumnView;
    Mode m_mode;

    DolphinMainWindow* m_mainWindow;
    QVBoxLayout* m_topLayout;

    DolphinController* m_controller;
    DolphinIconsView* m_iconsView;
    DolphinDetailsView* m_detailsView;
    DolphinColumnView* m_columnView;
    KFileItemDelegate* m_fileItemDelegate;

    KDirModel* m_dirModel;
    KDirLister* m_dirLister;
    DolphinSortFilterProxyModel* m_proxyModel;

    QList<CutItem> m_cutItemsCache;
};

#endif // DOLPHINVIEW_H
