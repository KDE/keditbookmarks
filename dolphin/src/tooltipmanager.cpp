/*******************************************************************************
 *   Copyright (C) 2008 by Konstantin Heil <konst.heil@stud.uni-heidelberg.de> *
 *                                                                             *
 *   This program is free software; you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation; either version 2 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program; if not, write to the                             *
 *   Free Software Foundation, Inc.,                                           *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA                *
 *******************************************************************************/

#include "tooltipmanager.h"

#include "dolphintooltip.h"
#include "dolphinmodel.h"
#include "dolphinsortfilterproxymodel.h"

#include <kicon.h>
#include <ktooltip.h>
#include <kio/previewjob.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QToolTip>

const int PREVIEW_WIDTH = 256;
const int PREVIEW_HEIGHT = 256;
const int ICON_WIDTH = 128;
const int ICON_HEIGHT = 128;
const int PREVIEW_DELAY = 250;

K_GLOBAL_STATIC(DolphinBalloonTooltipDelegate, g_delegate)

ToolTipManager::ToolTipManager(QAbstractItemView* parent,
                               DolphinSortFilterProxyModel* model) :
    QObject(parent),
    m_view(parent),
    m_dolphinModel(0),
    m_proxyModel(model),
    m_timer(0),
    m_previewTimer(0),
    m_waitOnPreviewTimer(0),
    m_item(),
    m_itemRect(),
    m_preview(),
    m_generatingPreview(),
    m_previewIsLate(),
    m_previewPass(),
    m_emptyRenderedKToolTipItem(),
    m_pix()
{
    KToolTip::setToolTipDelegate(g_delegate);

    m_dolphinModel = static_cast<DolphinModel*>(m_proxyModel->sourceModel());
    connect(parent, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(requestToolTip(const QModelIndex&)));
    connect(parent, SIGNAL(viewportEntered()),
            this, SLOT(hideToolTip()));

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(prepareToolTip()));

    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    connect(m_previewTimer, SIGNAL(timeout()),
            this, SLOT(startPreviewJob()));

    m_waitOnPreviewTimer = new QTimer(this);
    m_waitOnPreviewTimer->setSingleShot(true);
    connect(m_waitOnPreviewTimer, SIGNAL(timeout()),
            this, SLOT(prepareToolTip()));

    m_view->viewport()->installEventFilter(this);

    m_preview = false;
    m_generatingPreview = false;
    m_previewIsLate = false;
    m_previewPass = 0;
    m_pix = QPixmap();
}

ToolTipManager::~ToolTipManager()
{
}

void ToolTipManager::hideTip()
{
    hideToolTip();
}

bool ToolTipManager::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_view->viewport()) && (event->type() == QEvent::Leave)) {
        hideToolTip();
    }

    return QObject::eventFilter(watched, event);
}

void ToolTipManager::requestToolTip(const QModelIndex& index)
{
    if (index.column() == DolphinModel::Name) {
        KToolTip::hideTip();

        m_itemRect = m_view->visualRect(index);
        const QPoint pos = m_view->viewport()->mapToGlobal(m_itemRect.topLeft());
        m_itemRect.moveTo(pos);

        const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
        m_item = m_dolphinModel->itemForIndex(dirIndex);

        // Only start the previewJob when the mouse has been over this item for 200msec,
        // this prevents a lot of useless previewJobs (when passing rapidly over a lot of items).
        m_previewTimer->start(200);
        // reset these variables
        m_preview = false;
        m_previewIsLate = false;
        m_previewPass = 0;

        m_timer->start(500);
    } else {
        hideToolTip();
    }
}

void ToolTipManager::hideToolTip()
{
    m_timer->stop();
    m_previewTimer->stop();
    KToolTip::hideTip();
}

void ToolTipManager::prepareToolTip()
{
    if (m_generatingPreview) {
        if (m_previewPass == 1) {
            // We waited 250msec and the preview is still not finished,
            // so show the toolTip with a transparent image of maximal width.
            // When the preview finishes, m_previewIsLate will cause
            // a direct update of the tooltip, via m_emptyRenderedKToolTipItem.
            QPixmap paddedImage(QSize(PREVIEW_WIDTH, 32));
            m_previewIsLate = true;
            paddedImage.fill(Qt::transparent);
            KToolTipItem* toolTip = new KToolTipItem(paddedImage, m_item.getToolTipText());
            m_emptyRenderedKToolTipItem = toolTip; // make toolTip accessible everywhere
            showToolTip(toolTip);
        }
        
        ++m_previewPass;
        m_waitOnPreviewTimer->start(250);
    } else {
        // The preview generation has finished, find out under which circumstances.
        if (m_preview && m_previewIsLate) {
            // We got a preview, but it is late, the tooltip has already been shown.
            // So update the tooltip directly.
            m_emptyRenderedKToolTipItem->setData(Qt::DecorationRole, KIcon(m_pix));
            return;
        }

        KIcon icon;
        if (m_preview) {
            // We got a preview.
            icon = KIcon(m_pix);
        } else {
            // No preview, so use an icon.
            // Force a 128x128 icon, a 256x256 one is far too big.
            icon = KIcon(KIcon(m_item.iconName()).pixmap(ICON_WIDTH, ICON_HEIGHT));
        }

        KToolTipItem* toolTip = new KToolTipItem(icon, m_item.getToolTipText());
        showToolTip(toolTip);
    }
}

void ToolTipManager::showToolTip(KToolTipItem* tip)
{
    KStyleOptionToolTip option;
    // TODO: get option content from KToolTip or add KToolTip::sizeHint() method
    option.direction      = QApplication::layoutDirection();
    option.fontMetrics    = QFontMetrics(QToolTip::font());
    option.activeCorner   = KStyleOptionToolTip::TopLeftCorner;
    option.palette        = QToolTip::palette();
    option.font           = QToolTip::font();
    option.rect           = QRect();
    option.state          = QStyle::State_None;
    option.decorationSize = QSize(32, 32);

    QSize size;
    if (m_previewIsLate) {
        QPixmap paddedImage(QSize(PREVIEW_WIDTH, PREVIEW_HEIGHT));
        KToolTipItem* maxiTip = new KToolTipItem(paddedImage, m_item.getToolTipText());
        size = g_delegate->sizeHint(&option, maxiTip);
        delete maxiTip;
        maxiTip = 0;
    }
    else {
        size = g_delegate->sizeHint(&option, tip);
    }
    const QRect desktop = QApplication::desktop()->screenGeometry(m_itemRect.bottomRight());

    // m_itemRect defines the area of the item, where the tooltip should be
    // shown. Per default the tooltip is shown in the bottom right corner.
    // If the tooltip content exceeds the desktop borders, it must be assured that:
    // - the content is fully visible
    // - the content is not drawn inside m_itemRect
    const bool hasRoomToLeft  = (m_itemRect.left()   - size.width()  >= desktop.left());
    const bool hasRoomToRight = (m_itemRect.right()  + size.width()  <= desktop.right());
    const bool hasRoomAbove   = (m_itemRect.top()    - size.height() >= desktop.top());
    const bool hasRoomBelow   = (m_itemRect.bottom() + size.height() <= desktop.bottom());    
    if (!hasRoomAbove && !hasRoomBelow && !hasRoomToLeft && !hasRoomToRight) {
        delete tip;
        tip = 0;
        return;
    }

    int x = 0;
    if (hasRoomToLeft || hasRoomToRight) {
        x = hasRoomToRight ? m_itemRect.right() : m_itemRect.left() - size.width();
    } else {
        // Put the tooltip at the far right of the screen. The item will be overlapped
        // horizontally, but the y-coordinate will be adjusted afterwards so that no overlapping
        // occurs vertically.
        x = desktop.right() - size.width();
    }
    
    int y = 0;
    if (hasRoomBelow || hasRoomAbove) {
        y = hasRoomBelow ? m_itemRect.bottom() : m_itemRect.top() - size.height();
    } else {
        // Put the tooltip at the bottom of the screen. The x-coordinate has already
        // been adjusted, so that no overlapping with m_itemRect occurs.
        y = desktop.bottom() - size.height();
    }

    KToolTip::showTip(QPoint(x, y), tip);
}



void ToolTipManager::startPreviewJob()
{
    m_generatingPreview = true;
    KIO::PreviewJob* job = KIO::filePreview(KUrl::List() << m_item.url(),
                                            PREVIEW_WIDTH,
                                            PREVIEW_HEIGHT);
    job->setIgnoreMaximumSize(true);

    connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
            this, SLOT(setPreviewPix(const KFileItem&, const QPixmap&)));
    connect(job, SIGNAL(failed(const KFileItem&)),
            this, SLOT(previewFailed(const KFileItem&)));
}


void ToolTipManager::setPreviewPix(const KFileItem& item,
                                   const QPixmap& pixmap)
{
    if (m_item.url() != item.url()) {
        return;
    }
    
    QPixmap icon = pixmap;
    // only paint borders if the pixmap is opaque
    if (!icon.hasAlphaChannel()) {
        // TODO: Make code from IconManager for drawing a border accessible for
        // other classes (the following code has been adapted from IconManager).        
        // The frame is painted on top of the pixmap, this is needed to keep 
        // the text preview visually nice (the previewer adds ugly borders).
        QPainter painter;
        // make a buffer pixmap, tends to crash when 'icon' is directly painted ...
        QPixmap framedIcon(icon.size().width(), pixmap.size().height());
        framedIcon.fill();
        const int width = framedIcon.width() - 1;
        const int height = framedIcon.height() - 1;

        // draw the pixmap
        painter.begin(&framedIcon);
        painter.drawPixmap(0,0, icon);

        // draw the frame
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QColor(0, 0, 0));
        painter.drawRect(0, 0, width - 0, height - 1);
        painter.drawRect(1, 1, width - 2, height - 2);
        painter.setPen(QColor(255, 255, 255));
        painter.drawRect(2, 2, width - 4, height - 4);

        painter.end();
        icon = framedIcon;

        // provide an alpha channel for the frame
        QPixmap alphaChannel(icon.size());
        alphaChannel.fill();

        QPainter alphaPainter(&alphaChannel);
        alphaPainter.setBrush(Qt::NoBrush);
        alphaPainter.setRenderHint(QPainter::Antialiasing, false);
        alphaPainter.setPen(QColor(32, 32, 32));
        alphaPainter.drawRect(0, 0, width, height);
        alphaPainter.setPen(QColor(64, 64, 64));
        alphaPainter.drawRect(1, 1, width - 2, height - 2);

        icon.setAlphaChannel(alphaChannel);
    }

    if (m_previewIsLate) {
        // always use the maximal width
        QPixmap paddedImage(QSize(PREVIEW_WIDTH, icon.height()));
        paddedImage.fill(Qt::transparent);
        QPainter painter(&paddedImage);
        painter.drawPixmap((PREVIEW_WIDTH - icon.width()) / 2, 0, icon);
        m_pix = paddedImage;
    } else {
        m_pix = icon;
    }
    m_preview = true;
    m_generatingPreview = false;
}

void ToolTipManager::previewFailed(const KFileItem& item)
{
    Q_UNUSED(item);
    m_generatingPreview = false;
}

#include "tooltipmanager.moc"
