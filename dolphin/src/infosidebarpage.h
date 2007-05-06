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

#ifndef INFOSIDEBARPAGE_H
#define INFOSIDEBARPAGE_H

#include <sidebarpage.h>

#include <QtGui/QPushButton>
#include <QPixmap>
#include <QEvent>
#include <QLabel>
#include <QList>

#include <kurl.h>
#include <kmimetype.h>
#include <kdesktopfileactions.h>
#include <kvbox.h>

namespace KIO
{
    class Job;
}

class QPixmap;
class QIcon;
class QString;
class QPainter;
class KFileItem;
class QLabel;
class KVBox;
class PixmapViewer;
class MetaDataWidget;

/**
 * @brief Sidebar for showing meta information of one ore more selected items.
 */
class InfoSidebarPage : public SidebarPage
{
    Q_OBJECT

public:
    explicit InfoSidebarPage(QWidget* parent = 0);
    virtual ~InfoSidebarPage();

public slots:
    virtual void setUrl(const KUrl& url);
    virtual void setSelection(const KFileItemList& selection);

protected:
    /** @see QWidget::showEvent() */
    virtual void showEvent(QShowEvent* event);

private slots:
    /**
     * Does a delayed request of information for the item of the given Url and
     * provides default actions.
     *
     * @see InfoSidebarPage::showItemInfo()
     */
    void requestDelayedItemInfo(const KUrl& url);

    /**
     * Shows the information for the item of the Url which has been provided by
     * InfoSidebarPage::requestItemInfo() and provides default actions.
     */
    void showItemInfo();

    /**
     * Triggered if the request for item information has timed out.
     * @see InfoSidebarPage::requestDelayedItemInfo()
     */
    void slotTimeout();

    /**
     * Is invoked if no preview is available for the item. In this
     * case the icon will be shown.
     */
    void showIcon(const KFileItem& item);

    /**
     * Is invoked if a preview is available for the item. The preview
     * \a pixmap is shown inside the info page.
     */
    void showPreview(const KFileItem& item, const QPixmap& pixmap);

private:
    /**
     * Checks whether the an Url is repesented by a bookmark. If yes,
     * then the bookmark icon and name are shown instead of a preview.
     * @return True, if the Url represents exactly a bookmark.
     * @param url The url to check.
     */
    bool applyBookmark(const KUrl& url);

    /** Assures that any pending item information request is cancelled. */
    void cancelRequest();

    // TODO: the following methods are just a prototypes for meta
    // info generation...
    void createMetaInfo();
    void addInfoLine(const QString& labelText,
                     const QString& infoText);
    void beginInfoLines();
    void endInfoLines();

    /**
     * Returns true, if the string \a key represents a meta information
     * that should be shown.
     */
    bool showMetaInfo(const QString& key) const;

private:
    bool m_multipleSelection;
    bool m_pendingPreview;
    QTimer* m_timer;
    KUrl m_shownUrl;
    KUrl m_urlCandidate;

    PixmapViewer* m_preview;
    QLabel* m_name;

    QString m_infoLines;
    QLabel* m_infos;

    MetaDataWidget* m_metadataWidget;
};

#endif // INFOSIDEBARPAGE_H
