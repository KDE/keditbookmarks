/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#ifndef DOLPHINCONTROLLER_H
#define DOLPHINCONTROLLER_H

#include <dolphinview.h>
#include <kurl.h>
#include <QtCore/QObject>
#include <libdolphin_export.h>

class KUrl;
class QBrush;
class QModelIndex;
class QPoint;
class QRect;
class QWidget;

/**
 * @brief Acts as mediator between the abstract Dolphin view and the view
 *        implementations.
 *
 * The abstract Dolphin view (see DolphinView) represents the parent of the controller.
 * The controller is passed to the current view implementation
 * (see DolphinIconsView, DolphinDetailsView and DolphinColumnView)
 * by passing it in the constructor:
 *
 * \code
 * DolphinController* controller = new DolphinController(parent);
 * QAbstractItemView* view = new DolphinIconsView(parent, controller);
 * \endcode
 *
 * The communication of the view implementations to the abstract view is done by:
 * - triggerContextMenuRequest()
 * - requestActivation()
 * - indicateDroppedUrls()
 * - indicateSortingChange()
 * - indicateSortOrderChanged()
 * - setZoomInPossible()
 * - setZoomOutPossible()
 * - triggerItem()
 * - emitItemEntered()
 * - emitViewportEntered()
 *
 * The communication of the abstract view to the view implementations is done by:
 * - setShowHiddenFiles()
 * - setShowPreview()
 * - setAdditionalInfoCount()
 * - indicateActivationChange()
 * - triggerZoomIn()
 * - triggerZoomOut()
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinController : public QObject
{
    Q_OBJECT

public:
    explicit DolphinController(QObject* parent);
    virtual ~DolphinController();

    /** Sets the URL to \a url and emits the signal urlChanged(). */
    void setUrl(const KUrl& url);
    const KUrl& url() const;

    /**
     * Requests a context menu for the position \a pos. This method
     * should be invoked by the view implementation when a context
     * menu should be opened. The abstract Dolphin view itself
     * takes care itself to get the selected items depending from
     * \a pos.
     */
    void triggerContextMenuRequest(const QPoint& pos);

    /**
     * Requests an activation of the view and emits the signal
     * activated(). This method should be invoked by the view implementation
     * if e. g. a mouse click on the view has been done.
     * After the activation has been changed, the view implementation
     * might listen to the activationChanged() signal.
     */
    void requestActivation();

    /**
     * Indicates that URLs are dropped above a destination. This method
     * should be invoked by the view implementation. The abstract Dolphin view
     * will start the corresponding action (copy, move, link).
     * @param urls      URLs that are dropped above a destination.
     * @param destPath  Path of the destination.
     * @param destIndex Model index of the destination item.
     * @param source    Pointer to the view implementation which invoked this method.
     */
    void indicateDroppedUrls(const KUrl::List& urls,
                             const KUrl& destPath,
                             const QModelIndex& destIndex,
                             QWidget* source);

    /**
     * Informs the abstract Dolphin view about a sorting change done inside
     * the view implementation. This method should be invoked by the view
     * implementation (e. g. the details view uses this method in combination
     * with the details header).
     */
    void indicateSortingChange(DolphinView::Sorting sorting);

    /**
     * Informs the abstract Dolphin view about a sort order change done inside
     * the view implementation. This method should be invoked by the view
     * implementation (e. g. the details view uses this method in combination
     * with the details header).
     */
    void indicateSortOrderChange(Qt::SortOrder order);

    /**
     * Informs the view implementation about a change of the show hidden files
     * state and is invoked by the abstract Dolphin view.
     * The signal showHiddenFilesChanged() is emitted.
     */
    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const;

    /**
     * Informs the view implementation about a change of the show preview
     * state and is invoked by the abstract Dolphin view.
     * The signal showPreviewChanged() is emitted.
     */
    void setShowPreview(bool show);
    bool showPreview() const;

    /**
     * Informs the view implementation about a change of the number of
     * additional informations and is invoked by the abstract Dolphin view.
     * The signal additionalInfoCountChanged() is emitted.
     */
    void setAdditionalInfoCount(int count);
    bool additionalInfoCount() const;

    /**
     * Informs the view implementation about a change of the activation
     * state and is invoked by the abstract Dolphin view. The signal
     * activationChanged() is emitted.
     */
    void indicateActivationChange(bool active);

    /**
     * Tells the view implementation to zoom in by emitting the signal zoomIn()
     * and is invoked by the abstract Dolphin view.
     */
    void triggerZoomIn();

    /**
     * Is invoked by the view implementation to indicate whether a zooming in
     * is possible. The abstract Dolphin view updates the corresponding menu
     * action depending on this state.
     */
    void setZoomInPossible(bool possible);
    bool isZoomInPossible() const;

    /**
     * Tells the view implementation to zoom out by emitting the signal zoomOut()
     * and is invoked by the abstract Dolphin view.
     */
    void triggerZoomOut();

    /**
     * Is invoked by the view implementation to indicate whether a zooming out
     * is possible. The abstract Dolphin view updates the corresponding menu
     * action depending on this state.
     */
    void setZoomOutPossible(bool possible);
    bool isZoomOutPossible() const;

    // TODO: remove this method when the issue #160611 is solved in Qt 4.4
    static void drawHoverIndication(QWidget* widget,
                                    const QRect& bounds,
                                    const QBrush& brush);

public slots:
    /**
     * Emits the signal itemTriggered(). The method should be invoked by the
     * controller parent whenever the user has triggered an item. */
    void triggerItem(const KFileItem& item);

    /**
     * Emits the signal itemEntered(). The method should be invoked by
     * the controller parent whenever the mouse cursor is above an item.
     */
    void emitItemEntered(const KFileItem& item);

    /**
     * Emits the signal viewportEntered(). The method should be invoked by
     * the controller parent whenever the mouse cursor is above the viewport.
     */
    void emitViewportEntered();

signals:
    /**
     * Is emitted if the URL for the Dolphin controller has been changed
     * to \a url.
     */
    void urlChanged(const KUrl& url);

    /**
     * Is emitted if a context menu should be opened (see triggerContextMenuRequest()).
     * The abstract Dolphin view connects to this signal and will open the context menu.
     * @param pos       Position relative to the view widget where the
     *                  context menu should be opened. It is recommended
     *                  to get the corresponding model index from
     *                  this position.
     */
    void requestContextMenu(const QPoint& pos);

    /**
     * Is emitted if the view has been activated by e. g. a mouse click.
     * The abstract Dolphin view connects to this signal to know the
     * destination view for the menu actions.
     */
    void activated();

    /**
     * Is emitted if the URLs \a urls have been dropped to the destination
     * path \a destPath. If the URLs have been dropped above an item of
     * the destination path, the item is indicated by \a destIndex.
     * \a source indicates the widget where the dragging has been started from.
     */
    void urlsDropped(const KUrl::List& urls,
                     const KUrl& destPath,
                     const QModelIndex& destIndex,
                     QWidget* source);

    /**
     * Is emitted if the sorting has been changed to \a sorting by
     * the view implementation (see indicateSortingChanged().
     * The abstract Dolphin view connects to
     * this signal to update its menu action.
     */
    void sortingChanged(DolphinView::Sorting sorting);

    /**
     * Is emitted if the sort order has been changed to \a order
     * by the view implementation (see indicateSortOrderChanged().
     * The abstract Dolphin view connects
     * to this signal to update its menu actions.
     */
    void sortOrderChanged(Qt::SortOrder order);

    /**
     * Is emitted if the state for showing hidden files has been
     * changed to \a show by the abstract Dolphin view. The view
     * implementation might connect to this signal if custom
     * updates are required in this case.
     */
    void showHiddenFilesChanged(bool show);

    /**
     * Is emitted if the state for showing previews has been
     * changed to \a show by the abstract Dolphin view.
     * The view implementation might connect to this signal if custom
     * updates are required in this case.
     */
    void showPreviewChanged(bool show);

    /**
     * Is emitted if the number of additional informations has been
     * changed to \a count by the abstract Dolphin view.
     * The view implementation might connect to this signal if custom
     * updates are required in this case.
     */
    void additionalInfoCountChanged(int count);

    /**
     * Is emitted if the activation state has been changed to \a active
     * by the abstract Dolphin view.
     * The view implementation might connect to this signal if custom
     * updates are required in this case.
     */
    void activationChanged(bool active);

    /**
     * Is emitted if the item \a item should be triggered. The abstract
     * Dolphin view connects to this signal. If the item represents a directory,
     * the directory is opened. On a file the corresponding application is opened.
     * The item is null (see KFileItem::isNull()), when clicking on the viewport itself.
     */
    void itemTriggered(const KFileItem& item);

    /**
     * Is emitted if the mouse cursor has entered the item
     * given by \a index (see emitItemEntered()).
     * The abstract Dolphin view connects to this signal.
     */
    void itemEntered(const KFileItem& item);

    /**
     * Is emitted if the mouse cursor has entered
     * the viewport (see emitViewportEntered().
     * The abstract Dolphin view connects to this signal.
     */
    void viewportEntered();

    /**
     * Is emitted if the view should zoom in. The view implementation
     * must connect to this signal if it supports zooming.
     */
    void zoomIn();

    /**
     * Is emitted if the view should zoom out. The view implementation
     * must connect to this signal if it supports zooming.
     */
    void zoomOut();

private:
    bool m_showHiddenFiles;
    bool m_showPreview;
    bool m_zoomInPossible;
    bool m_zoomOutPossible;
    int m_additionalInfoCount;
    KUrl m_url;
};

inline const KUrl& DolphinController::url() const
{
    return m_url;
}

inline bool DolphinController::showHiddenFiles() const
{
    return m_showHiddenFiles;
}

inline bool DolphinController::showPreview() const
{
    return m_showPreview;
}

inline bool DolphinController::additionalInfoCount() const
{
    return m_additionalInfoCount;
}

inline void DolphinController::setZoomInPossible(bool possible)
{
    m_zoomInPossible = possible;
}

inline bool DolphinController::isZoomInPossible() const
{
    return m_zoomInPossible;
}

inline void DolphinController::setZoomOutPossible(bool possible)
{
    m_zoomOutPossible = possible;
}

inline bool DolphinController::isZoomOutPossible() const
{
    return m_zoomOutPossible;
}

#endif
