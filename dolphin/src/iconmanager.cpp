/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "iconmanager.h"

#include "dolphinmodel.h"

#include <kiconeffect.h>
#include <kio/previewjob.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <konqmimedata.h>

#include <QApplication>
#include <QClipboard>
#include <QIcon>

IconManager::IconManager(QObject* parent, DolphinModel* model) :
    QObject(parent),
    m_showPreview(false),
    m_previewJobs(),
    m_dolphinModel(model),
    m_cutItemsCache()
{
    connect(model->dirLister(), SIGNAL(newItems(const KFileItemList&)),
            this, SLOT(generatePreviews(const KFileItemList&)));

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updateCutItems()));
}

IconManager::~IconManager()
{
    foreach (KJob* job, m_previewJobs) {
        Q_ASSERT(job != 0);
        job->kill();
    }
    m_previewJobs.clear();
}


void IconManager::setShowPreview(bool show)
{
    if (m_showPreview != show) {
        m_showPreview = show;
        m_cutItemsCache.clear();
        updateCutItems();
    }
}

void IconManager::generatePreviews(const KFileItemList& items)
{
    if (!m_showPreview) {
        return;
    }

    KIO::PreviewJob* job = KIO::filePreview(items, 128);
    connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
            this, SLOT(replaceIcon(const KFileItem&, const QPixmap&)));
    connect(job, SIGNAL(finished(KJob*)),
            this, SLOT(slotPreviewJobFinished(KJob*)));

    m_previewJobs.append(job);
}

void IconManager::replaceIcon(const KFileItem& item, const QPixmap& pixmap)
{
    Q_ASSERT(!item.isNull());
    KDirLister* dirLister = m_dolphinModel->dirLister();
    if (!m_showPreview || (item.url().directory() != dirLister->url().path())) {
        // the preview has been canceled in the meanwhile or the preview
        // job is still working on items of an older URL, hence
        // the item is not part of the directory model anymore
        return;
    }

    const QModelIndex idx = m_dolphinModel->indexForItem(item);
    if (idx.isValid() && (idx.column() == 0)) {
        const QMimeData* mimeData = QApplication::clipboard()->mimeData();
        if (KonqMimeData::decodeIsCutSelection(mimeData) && isCutItem(item)) {
            KIconEffect iconEffect;
            const QPixmap cutPixmap = iconEffect.apply(pixmap, KIconLoader::Desktop, KIconLoader::DisabledState);
            m_dolphinModel->setData(idx, QIcon(cutPixmap), Qt::DecorationRole);
        } else {
            m_dolphinModel->setData(idx, QIcon(pixmap), Qt::DecorationRole);
        }
    }
}


void IconManager::slotPreviewJobFinished(KJob* job)
{
    const int index = m_previewJobs.indexOf(job);
    m_previewJobs.removeAt(index);
}

void IconManager::updateCutItems()
{
    // restore the icons of all previously selected items to the
    // original state...
    foreach (CutItem cutItem, m_cutItemsCache) {
        const QModelIndex index = m_dolphinModel->indexForUrl(cutItem.url);
        if (index.isValid()) {
            m_dolphinModel->setData(index, QIcon(cutItem.pixmap), Qt::DecorationRole);
        }
    }
    m_cutItemsCache.clear();

    // ... and apply an item effect to all currently cut items
    applyCutItemEffect();
}

bool IconManager::isCutItem(const KFileItem& item) const
{
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    const KUrl::List cutUrls = KUrl::List::fromMimeData(mimeData);

    const KUrl& itemUrl = item.url();
    foreach (KUrl url, cutUrls) {
        if (url == itemUrl) {
            return true;
        }
    }

    return false;
}

void IconManager::applyCutItemEffect()
{
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    if (!KonqMimeData::decodeIsCutSelection(mimeData)) {
        return;
    }

    const KFileItemList items(m_dolphinModel->dirLister()->items());
    foreach (KFileItem item, items) {
        if (isCutItem(item)) {
            const QModelIndex index = m_dolphinModel->indexForItem(item);
            const QVariant value = m_dolphinModel->data(index, Qt::DecorationRole);
            if (value.type() == QVariant::Icon) {
                const QIcon icon(qvariant_cast<QIcon>(value));
                QPixmap pixmap = icon.pixmap(128, 128);

                // remember current pixmap for the item to be able
                // to restore it when other items get cut
                CutItem cutItem;
                cutItem.url = item.url();
                cutItem.pixmap = pixmap;
                m_cutItemsCache.append(cutItem);

                // apply icon effect to the cut item
                KIconEffect iconEffect;
                pixmap = iconEffect.apply(pixmap, KIconLoader::Desktop, KIconLoader::DisabledState);
                m_dolphinModel->setData(index, QIcon(pixmap), Qt::DecorationRole);
            }
        }
    }
}

#include "iconmanager.moc"
