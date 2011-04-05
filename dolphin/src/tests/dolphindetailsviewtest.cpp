/***************************************************************************
 *   Copyright (C) 2010 by Frank Reininghaus (frank78ac@googlemail.com)    *
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

#include <qtest_kde.h>

#include "testbase.h"
#include "testdir.h"

#include "views/dolphindetailsview.h"
#include "views/dolphinview.h"
#include "views/dolphinmodel.h"
#include "views/dolphinsortfilterproxymodel.h"
#include "views/zoomlevelinfo.h"

#include <qtestmouse.h>
#include <qtestkeyboard.h>

class DolphinDetailsViewTest : public TestBase
{
    Q_OBJECT

private slots:

    void testExpandedUrls();

    void bug217447_shiftArrowSelection();
    void bug234600_overlappingIconsWhenZooming();
    void bug257401_longFilenamesKeyboardNavigation();

private:

    /**
     * initView(DolphinView*) sets the correct view mode, shows the view on the screen, and waits
     * until loading the folder in the view is finished.
     *
     * Many unit tests need access to the internal DolphinDetailsView in DolphinView.
     * Therefore, a pointer to the details view is returned by initView(DolphinView*).
     */
    DolphinDetailsView* initView(DolphinView* view) const {
        view->setMode(DolphinView::DetailsView);
        DolphinDetailsView* detailsView = qobject_cast<DolphinDetailsView*>(itemView(view));
        detailsView->setFoldersExpandable(true);
        view->resize(400, 400);
        view->show();
        QTest::qWaitForWindowShown(view);
        reloadViewAndWait(view);
        return detailsView;
    }

    QModelIndex proxyModelIndexForUrl(const DolphinView* view, const KUrl& url) const {
        const QModelIndex index = view->m_viewAccessor.m_dolphinModel->indexForUrl(url);
        return view->m_viewAccessor.m_proxyModel->mapFromSource(index);
    }
};

/**
 * This test verifies that DolphinDetailsView::expandedUrls() returns the right set of URLs.
 * The test creates a folder hierarchy: 3 folders (a, b, c) contain 3 subfolders (also named a, b, c) each.
 * Each of those contains 3 further subfolders of the same name.
 */

void DolphinDetailsViewTest::testExpandedUrls()
{
    QStringList files;
    QStringList subFolderNames;
    subFolderNames << "a" << "b" << "c";

    foreach(const QString& level1, subFolderNames) {
        foreach(const QString& level2, subFolderNames) {
            foreach(const QString& level3, subFolderNames) {
                files << level1 + "/" + level2 + "/" + level3 + "/testfile";
            }
        }
    }

    TestDir dir;
    dir.createFiles(files);
    DolphinView view(dir.url(), 0);
    DolphinDetailsView* detailsView = initView(&view);

    // We start with an empty set of expanded URLs.
    QSet<KUrl> expectedExpandedUrls;
    QCOMPARE(detailsView->expandedUrls(), expectedExpandedUrls);

    // Expand URLs one by one and verify the result of DolphinDetailsView::expandedUrls()
    QStringList itemsToExpand;
    itemsToExpand << "b" << "b/a" << "b/a/c" << "b/c" << "c";

    foreach(const QString& item, itemsToExpand) {
        KUrl url(dir.name() + item);
        detailsView->expand(proxyModelIndexForUrl(&view, url));
        expectedExpandedUrls += url;
        QCOMPARE(detailsView->expandedUrls(), expectedExpandedUrls);

        // Before we proceed, we have to make sure that the view has finished
        // loading the contents of the expanded folder.
        QVERIFY(waitForFinishedPathLoading(&view));
    }

    // Collapse URLs one by one and verify the result of DolphinDetailsView::expandedUrls()
    QStringList itemsToCollapse;
    itemsToCollapse << "b/c" << "b/a/c" << "c" << "b/a" << "b";

    foreach(const QString& item, itemsToCollapse) {
        KUrl url(dir.name() + item);
        detailsView->collapse(proxyModelIndexForUrl(&view, url));
        expectedExpandedUrls -= url;
        QCOMPARE(detailsView->expandedUrls(), expectedExpandedUrls);
    }
}

/**
 * When the first item in the view is active and Shift is held while the "arrow down"
 * key is pressed repeatedly, the selection should grow by one item for each key press.
 * A change in Qt 4.6 revealed a bug in DolphinDetailsView which broke this, see
 *
 * https://bugs.kde.org/show_bug.cgi?id=217447
 *
 * The problem was that DolphinDetailsView, which uses not the full width of the "Name"
 * column for an item, but only the width of the actual file name, did not reimplement
 * QTreeView::visualRect(). This caused item selection to fail because QAbstractItemView
 * uses the center of the visualRect of an item internally. If the width of the file name
 * is less than half the width of the "Name" column, the center of an item's visualRect
 * was therefore outside the space that DolphinDetailsView actually assigned to the
 * item, and this led to unexpected deselection of items.
 *
 * TODO: To make the test more reliable, one could adjust the width of the "Name"
 * column before the test in order to really make sure that the column is more than twice
 * as wide as the space actually occupied by the file names (this triggers the bug).
 */

void DolphinDetailsViewTest::bug217447_shiftArrowSelection()
{
    TestDir dir;
    for (int i = 0; i < 100; i++) {
        dir.createFile(QString("%1").arg(i));
    }
    DolphinView view(dir.url(), 0);
    DolphinDetailsView* detailsView = initView(&view);

    // Select the first item
    QModelIndex index0 = detailsView->model()->index(0, 0);
    detailsView->setCurrentIndex(index0);
    QCOMPARE(detailsView->currentIndex(), index0);

    // Before we test Shift-selection, we verify that the root cause is fixed a bit more
    // directly: we check that passing the corners or the center of an item's visualRect
    // to itemAt() returns the item (and not an invalid model index).
    QRect rect = detailsView->visualRect(index0);
    QCOMPARE(detailsView->indexAt(rect.center()), index0);
    QCOMPARE(detailsView->indexAt(rect.topLeft()), index0);
    QCOMPARE(detailsView->indexAt(rect.topRight()), index0);
    QCOMPARE(detailsView->indexAt(rect.bottomLeft()), index0);
    QCOMPARE(detailsView->indexAt(rect.bottomRight()), index0);

    // Another way to test this is to Ctrl-click the center of the visualRect.
    // The selection state of the item should be toggled.
    detailsView->clearSelection();
    QItemSelectionModel* selectionModel = detailsView->selectionModel();
    QCOMPARE(selectionModel->selectedIndexes().count(), 0);

    QTest::mouseClick(detailsView->viewport(), Qt::LeftButton, Qt::ControlModifier, rect.center());
    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
    QCOMPARE(selectedIndexes.count(), 1);
    QVERIFY(selectedIndexes.contains(index0));

    // Now we go down item by item using Shift+Down. In each step, we check that the current item
    // is added to the selection and that the size of the selection grows by one.

    int current = 1;

    while (current < 100) {
        QTest::keyClick(detailsView->viewport(), Qt::Key_Down, Qt::ShiftModifier);
        QModelIndex currentIndex = detailsView->model()->index(current, 0);
        QCOMPARE(detailsView->currentIndex(), currentIndex);

        selectedIndexes = selectionModel->selectedIndexes();
        QCOMPARE(selectedIndexes.count(), current + 1);
        QVERIFY(selectedIndexes.contains(currentIndex));

        current++;
    }
}

/**
 * When the icon size is changed, we have to make sure that the maximumSize given
 * to KFileItemDelegate for rendering each item is updated correctly. If this is not
 * done, the visualRects are clipped by the incorrect maximum size, and the icons
 * may overlap, see
 *
 * https://bugs.kde.org/show_bug.cgi?id=234600
 */

void DolphinDetailsViewTest::bug234600_overlappingIconsWhenZooming()
{
    QStringList files;
    files << "a" << "b" << "c" << "d";

    TestDir dir;
    dir.createFiles(files);
    DolphinView view(dir.url(), 0);
    DolphinDetailsView* detailsView = initView(&view);

    QModelIndex index0 = detailsView->model()->index(0, 0);
    detailsView->setCurrentIndex(index0);
    QCOMPARE(detailsView->currentIndex(), index0);

    // Setting the zoom level to the minimum value and triggering DolphinDetailsView::currentChanged(...)
    // should make sure that the bug is triggered.
    int zoomLevelBackup = view.zoomLevel();
    int zoomLevel = ZoomLevelInfo::minimumLevel();
    view.setZoomLevel(zoomLevel);

    QModelIndex index1 = detailsView->model()->index(1, 0);
    detailsView->setCurrentIndex(index1);
    QCOMPARE(detailsView->currentIndex(), index1);

    // Increase the zoom level successively to the maximum.
    while(zoomLevel < ZoomLevelInfo::maximumLevel()) {
        zoomLevel++;
        view.setZoomLevel(zoomLevel);
        QCOMPARE(view.zoomLevel(), zoomLevel);

        //Check for each zoom level that the height of each item is at least the icon size.
        QVERIFY(detailsView->visualRect(index1).height() >= ZoomLevelInfo::iconSizeForZoomLevel(zoomLevel));
    }

    view.setZoomLevel(zoomLevelBackup);
}

/**
 * The width of the visualRect of an item is usually replaced by the width of the file name.
 * However, if the file name is wider then the view's name column, this leads to problems with
 * keyboard navigation if files with very long names are present in the current folder, see
 *
 * https://bugs.kde.org/show_bug.cgi?id=257401
 *
 * This test checks that the visualRect of an item is never wider than the "Name" column.
 */

void DolphinDetailsViewTest::bug257401_longFilenamesKeyboardNavigation() {
    TestDir dir;
    QString name;
    for (int i = 0; i < 20; i++) {
        name += "mmmmmmmmmm";
        dir.createFile(name);
    }
    DolphinView view(dir.url(), 0);
    DolphinDetailsView* detailsView = initView(&view);

    // Select the first item
    QModelIndex index0 = detailsView->model()->index(0, 0);
    detailsView->setCurrentIndex(index0);
    QCOMPARE(detailsView->currentIndex(), index0);
    QVERIFY(detailsView->visualRect(index0).width() < detailsView->columnWidth(DolphinModel::Name));

    QItemSelectionModel* selectionModel = detailsView->selectionModel();
    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
    QCOMPARE(selectedIndexes.count(), 1);
    QVERIFY(selectedIndexes.contains(index0));

    // Move down successively using the "Down" key and check that current item
    // and selection are as expected.
    for (int i = 0; i < 19; i++) {
        QTest::keyClick(detailsView->viewport(), Qt::Key_Down, Qt::NoModifier);
        QModelIndex currentIndex = detailsView->model()->index(i + 1, 0);
        QCOMPARE(detailsView->currentIndex(), currentIndex);
        QVERIFY(detailsView->visualRect(currentIndex).width() <= detailsView->columnWidth(DolphinModel::Name));
        selectedIndexes = selectionModel->selectedIndexes();
        QCOMPARE(selectedIndexes.count(), 1);
        QVERIFY(selectedIndexes.contains(currentIndex));
    }
}

QTEST_KDEMAIN(DolphinDetailsViewTest, GUI)

#include "dolphindetailsviewtest.moc"
