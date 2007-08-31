/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   and Patrice Tremblay                                                  *
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

#include "statusbarspaceinfo.h"

#include <kcolorscheme.h>
#include <kdiskfreespace.h>
#include <kmountpoint.h>
#include <klocale.h>
#include <kio/job.h>

#include <QTimer>
#include <QPainter>
#include <QKeyEvent>

StatusBarSpaceInfo::StatusBarSpaceInfo(QWidget* parent) :
    QWidget(parent),
    m_gettingSize(false),
    m_kBSize(0),
    m_kBAvailable(0)
{
    setMinimumWidth(200);

    QPalette palette;
    palette.setColor(QPalette::Background, Qt::transparent);
    setPalette(palette);

    // Update the space information each 10 seconds. Polling is useful
    // here, as files can be deleted/added outside the scope of Dolphin.
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(refresh()));
    timer->start(10000);
}

StatusBarSpaceInfo::~StatusBarSpaceInfo()
{
}

void StatusBarSpaceInfo::setUrl(const KUrl& url)
{
    m_url = url;
    refresh();
    QTimer::singleShot(300, this, SLOT(update()));
}

void StatusBarSpaceInfo::paintEvent(QPaintEvent* /* event */)
{
    QPainter painter(this);
    const int barWidth = width();
    const int barTop = 1;
    const int barHeight = height() - 5;

    QString text;

    const int widthDec = 3;  // visual decrement for the available width

    QColor frameColor = palette().brush(QPalette::Background).color();
    frameColor.setAlpha(128);
    painter.setPen(frameColor);

    const QColor backgrColor = KColorScheme(KColorScheme::View).background();
    painter.setBrush(backgrColor);

    painter.drawRect(QRect(0, barTop + 1 , barWidth - widthDec, barHeight));

    if ((m_kBSize > 0) && (m_kBAvailable > 0)) {
        // draw 'used size' bar
        painter.setPen(Qt::NoPen);
        painter.setBrush(progressColor(backgrColor));
        int usedWidth = barWidth - static_cast<int>((m_kBAvailable *
                        static_cast<float>(barWidth)) / m_kBSize);
        const int left = 1;
        int right = usedWidth - widthDec;
        if (right < left) {
            right = left;
        }
        painter.drawRect(QRect(left, barTop + 2, right, barHeight - 1));

        text = i18nc("@info:status", "%1 free", KIO::convertSizeFromKiB(m_kBAvailable));
    } else {
        if (m_gettingSize) {
            text = i18nc("@info:status", "Getting size...");
        } else {
            text = QString();
            QMetaObject::invokeMethod(this, "hide", Qt::QueuedConnection);
        }
    }

    // draw text
    painter.setPen(KColorScheme(KColorScheme::View).foreground());
    painter.drawText(QRect(1, 1, barWidth - 2, barHeight + 6),
                     Qt::AlignCenter | Qt::TextWordWrap,
                     text);
}


void StatusBarSpaceInfo::slotFoundMountPoint(const QString& mountPoint,
                                             quint64 kBSize,
                                             quint64 kBUsed,
                                             quint64 kBAvailable)
{
    Q_UNUSED(kBUsed);
    Q_UNUSED(mountPoint);

    m_gettingSize = false;
    m_kBSize = kBSize;
    m_kBAvailable = kBAvailable;

    update();
}

void StatusBarSpaceInfo::showResult()
{
    m_gettingSize = false;
    update();
}

void StatusBarSpaceInfo::refresh()
{
    m_kBSize = 0;
    m_kBAvailable = 0;

    // KDiskFreeSpace is for local paths only
    if (!m_url.isLocalFile()) {
        return;
    }

    m_gettingSize = true;
    KMountPoint::Ptr mp = KMountPoint::currentMountPoints().findByPath(m_url.path());
    if (!mp)
        return;

    KDiskFreeSpace* job = new KDiskFreeSpace(this);
    connect(job, SIGNAL(foundMountPoint(const QString&,
                                        quint64,
                                        quint64,
                                        quint64)),
            this, SLOT(slotFoundMountPoint(const QString&,
                                           quint64,
                                           quint64,
                                           quint64)));
    connect(job, SIGNAL(done()),
            this, SLOT(showResult()));

    job->readDF(mp->mountPoint());
}

QColor StatusBarSpaceInfo::progressColor(const QColor& bgColor) const
{
    QColor color = KColorScheme(KColorScheme::Button).background();

    // assure that enough contrast is given between the background color
    // and the progressbar color
    int bgRed   = bgColor.red();
    int bgGreen = bgColor.green();
    int bgBlue  = bgColor.blue();

    const int backgrBrightness = qGray(bgRed, bgGreen, bgBlue);
    const int progressBrightness = qGray(color.red(), color.green(), color.blue());

    const int limit = 32;
    const int diff = backgrBrightness - progressBrightness;
    bool adjustColor = ((diff >= 0) && (diff <  limit)) ||
                       ((diff  < 0) && (diff > -limit));
    if (adjustColor) {
        const int inc = (backgrBrightness < 2 * limit) ? (2 * limit) : -limit;
        color = QColor(bgRed + inc, bgGreen + inc, bgBlue + inc);
    }

    return color;
}

#include "statusbarspaceinfo.moc"
