/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2011 by Frank Reininghaus <frank78ac@googlemail.com>    *
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

#include <KDirLister>
#include "kitemviews/kfileitemmodel.h"
#include "testdir.h"

namespace {
    const int DefaultTimeout = 5000;
};

Q_DECLARE_METATYPE(KItemRangeList)

class KFileItemModelTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testDefaultRoles();
    void testDefaultSortRole();
    void testDefaultGroupRole();
    void testNewItems();
    void testRemoveItems();
    void testModelConsistencyWhenInsertingItems();
    void testItemRangeConsistencyWhenInsertingItems();
    void testExpandItems();
    void testSorting();

    void testExpansionLevelsCompare_data();
    void testExpansionLevelsCompare();

    void testIndexForKeyboardSearch();

private:
    bool isModelConsistent() const;
    QStringList itemsInModel() const;

private:
    KFileItemModel* m_model;
    KDirLister* m_dirLister;
    TestDir* m_testDir;
};

void KFileItemModelTest::init()
{
    qRegisterMetaType<KItemRangeList>("KItemRangeList");
    qRegisterMetaType<KFileItemList>("KFileItemList");

    m_testDir = new TestDir();
    m_dirLister = new KDirLister();
    m_model = new KFileItemModel(m_dirLister);
}

void KFileItemModelTest::cleanup()
{
    delete m_model;
    m_model = 0;

    delete m_dirLister;
    m_dirLister = 0;

    delete m_testDir;
    m_testDir = 0;
}

void KFileItemModelTest::testDefaultRoles()
{
    const QSet<QByteArray> roles = m_model->roles();
    QCOMPARE(roles.count(), 2);
    QVERIFY(roles.contains("name"));
    QVERIFY(roles.contains("isDir"));
}

void KFileItemModelTest::testDefaultSortRole()
{
    QCOMPARE(m_model->sortRole(), QByteArray("name"));

    QStringList files;
    files << "c.txt" << "a.txt" << "b.txt";

    m_testDir->createFiles(files);

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_model->data(0)["name"].toString(), QString("a.txt"));
    QCOMPARE(m_model->data(1)["name"].toString(), QString("b.txt"));
    QCOMPARE(m_model->data(2)["name"].toString(), QString("c.txt"));
}

void KFileItemModelTest::testDefaultGroupRole()
{
    QVERIFY(m_model->groupRole().isEmpty());
}

void KFileItemModelTest::testNewItems()
{
    QStringList files;
    files << "a.txt" << "b.txt" << "c.txt";
    m_testDir->createFiles(files);

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    QCOMPARE(m_model->count(), 3);

    QVERIFY(isModelConsistent());
}

void KFileItemModelTest::testRemoveItems()
{
    m_testDir->createFile("a.txt");
    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 1);

    m_testDir->removeFile("a.txt");
    m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsRemoved(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 0);
}

void KFileItemModelTest::testModelConsistencyWhenInsertingItems()
{
    QSKIP("Temporary disabled", SkipSingle);

    // KFileItemModel prevents that inserting a punch of items sequentially
    // results in an itemsInserted()-signal for each item. Instead internally
    // a timeout is given that collects such operations and results in only
    // one itemsInserted()-signal. However in this test we want to stress
    // KFileItemModel to do a lot of insert operation and hence decrease
    // the timeout to 1 millisecond.
    m_model->m_minimumUpdateIntervalTimer->setInterval(1);

    m_testDir->createFile("1");
    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 1);

    // Insert 10 items for 20 times. After each insert operation the model consistency
    // is checked.
    QSet<int> insertedItems;
    for (int i = 0; i < 20; ++i) {
        QSignalSpy spy(m_model, SIGNAL(itemsInserted(KItemRangeList)));

        for (int j = 0; j < 10; ++j) {
            int itemName = qrand();
            while (insertedItems.contains(itemName)) {
                itemName = qrand();
            }
            insertedItems.insert(itemName);

            m_testDir->createFile(QString::number(itemName));
        }

        m_dirLister->updateDirectory(m_testDir->url());
        if (spy.count() == 0) {
            QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
        }

        QVERIFY(isModelConsistent());
    }

    QCOMPARE(m_model->count(), 201);
}

void KFileItemModelTest::testItemRangeConsistencyWhenInsertingItems()
{
    QStringList files;
    files << "B" << "E" << "G";
    m_testDir->createFiles(files);

    // Due to inserting the 3 items one item-range with index == 0 and
    // count == 3 must be given
    QSignalSpy spy1(m_model, SIGNAL(itemsInserted(KItemRangeList)));
    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    QCOMPARE(spy1.count(), 1);
    QList<QVariant> arguments = spy1.takeFirst();
    KItemRangeList itemRangeList = arguments.at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(0, 3));

    // The indexes of the item-ranges must always be related to the model before
    // the items have been inserted. Having:
    //   0 1 2
    //   B E G
    // and inserting A, C, D, F the resulting model will be:
    //   0 1 2 3 4 5 6
    //   A B C D E F G
    // and the item-ranges must be:
    //   index: 0, count: 1 for A
    //   index: 1, count: 2 for B, C
    //   index: 2, count: 1 for G

    files.clear();
    files << "A" << "C" << "D" << "F";
    m_testDir->createFiles(files);

    QSignalSpy spy2(m_model, SIGNAL(itemsInserted(KItemRangeList)));
    m_dirLister->updateDirectory(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    QCOMPARE(spy2.count(), 1);
    arguments = spy2.takeFirst();
    itemRangeList = arguments.at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(0, 1) << KItemRange(1, 2) << KItemRange(2, 1));
}

void KFileItemModelTest::testExpandItems()
{
    // Test expanding subfolders in a folder with the items "a/", "a/a/", "a/a/1", "a/a-1/", "a/a-1/1".
    // Besides testing the basic item expansion functionality, the test makes sure that
    // KFileItemModel::expansionLevelsCompare(const KFileItem& a, const KFileItem& b)
    // yields the correct result for "a/a/1" and "a/a-1/", whis is non-trivial because they share the
    // first three characters.
    QSet<QByteArray> modelRoles = m_model->roles();
    modelRoles << "isExpanded" << "expansionLevel";
    m_model->setRoles(modelRoles);

    QStringList files;
    files << "a/a/1" << "a/a-1/1"; // missing folders are created automatically
    m_testDir->createFiles(files);

    // Store the URLs of all folders in a set.
    QSet<KUrl> allFolders;
    allFolders << KUrl(m_testDir->name() + "a") << KUrl(m_testDir->name() + "a/a") << KUrl(m_testDir->name() + "a/a-1");

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    // So far, the model contains only "a/"
    QCOMPARE(m_model->count(), 1);
    QVERIFY(m_model->isExpandable(0));
    QVERIFY(!m_model->isExpanded(0));
    QVERIFY(m_model->expandedUrls().empty());

    QSignalSpy spyInserted(m_model, SIGNAL(itemsInserted(KItemRangeList)));

    // Expand the folder "a/" -> "a/a/" and "a/a-1/" become visible
    m_model->setExpanded(0, true);
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 3); // 3 items: "a/", "a/a/", "a/a-1/"
    QCOMPARE(m_model->expandedUrls(), QSet<KUrl>() << KUrl(m_testDir->name() + "a"));

    QCOMPARE(spyInserted.count(), 1);
    KItemRangeList itemRangeList = spyInserted.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(1, 2)); // 2 new items "a/a/" and "a/a-1/" with indices 1 and 2

    QVERIFY(m_model->isExpandable(1));
    QVERIFY(!m_model->isExpanded(1));
    QVERIFY(m_model->isExpandable(2));
    QVERIFY(!m_model->isExpanded(2));

    // Expand the folder "a/a/" -> "a/a/1" becomes visible
    m_model->setExpanded(1, true);
    QVERIFY(m_model->isExpanded(1));
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 4);  // 4 items: "a/", "a/a/", "a/a/1", "a/a-1/"
    QCOMPARE(m_model->expandedUrls(), QSet<KUrl>() << KUrl(m_testDir->name() + "a") << KUrl(m_testDir->name() + "a/a"));

    QCOMPARE(spyInserted.count(), 1);
    itemRangeList = spyInserted.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(2, 1)); // 1 new item "a/a/1" with index 2

    QVERIFY(!m_model->isExpandable(2));
    QVERIFY(!m_model->isExpanded(2));

    // Expand the folder "a/a-1/" -> "a/a-1/1" becomes visible
    m_model->setExpanded(3, true);
    QVERIFY(m_model->isExpanded(3));
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));
    QCOMPARE(m_model->count(), 5);  // 5 items: "a/", "a/a/", "a/a/1", "a/a-1/", "a/a-1/1"
    QCOMPARE(m_model->expandedUrls(), allFolders);

    QCOMPARE(spyInserted.count(), 1);
    itemRangeList = spyInserted.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(4, 1)); // 1 new item "a/a-1/1" with index 4

    QVERIFY(!m_model->isExpandable(4));
    QVERIFY(!m_model->isExpanded(4));

    QSignalSpy spyRemoved(m_model, SIGNAL(itemsRemoved(KItemRangeList)));

    // Collapse the top-level folder -> all other items should disappear
    m_model->setExpanded(0, false);
    QVERIFY(!m_model->isExpanded(0));
    QCOMPARE(m_model->count(), 1);
    QVERIFY(!m_model->expandedUrls().contains(KUrl(m_testDir->name() + "a"))); // TODO: Make sure that child URLs are also removed

    QCOMPARE(spyRemoved.count(), 1);
    itemRangeList = spyRemoved.takeFirst().at(0).value<KItemRangeList>();
    QCOMPARE(itemRangeList, KItemRangeList() << KItemRange(1, 4)); // 4 items removed

    // Clear the model, reload the folder and try to restore the expanded folders.
    m_model->clear();
    QCOMPARE(m_model->count(), 0);
    QVERIFY(m_model->expandedUrls().empty());

    m_dirLister->openUrl(m_testDir->url());
    m_model->restoreExpandedUrls(allFolders);
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(loadingCompleted()), DefaultTimeout));
    QCOMPARE(m_model->count(), 5);  // 5 items: "a/", "a/a/", "a/a/1", "a/a-1/", "a/a-1/1"
    QVERIFY(m_model->isExpanded(0));
    QVERIFY(m_model->isExpanded(1));
    QVERIFY(!m_model->isExpanded(2));
    QVERIFY(m_model->isExpanded(3));
    QVERIFY(!m_model->isExpanded(4));
    QCOMPARE(m_model->expandedUrls(), allFolders);
}

void KFileItemModelTest::testSorting()
{
    // Create some files with different sizes and modification times to check the different sorting options
    QDateTime now = QDateTime::currentDateTime();

    m_testDir->createFile("a", "A file", now.addDays(-3));
    m_testDir->createFile("b", "A larger file", now.addDays(0));
    m_testDir->createDir("c", now.addDays(-2));
    m_testDir->createFile("d", "The largest file in this directory", now.addDays(-1));
    m_testDir->createFile("e", "An even larger file", now.addDays(-4));
    m_testDir->createFile(".f");

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    // Default: Sort by Name, ascending
    QCOMPARE(m_model->sortRole(), QByteArray("name"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(m_model->sortFoldersFirst());
    //QVERIFY(!m_model->showHiddenFiles());
    QCOMPARE(itemsInModel(), QStringList() << "c" << "a" << "b" << "d" << "e");

    // Sort by Name, descending
    m_model->setSortOrder(Qt::DescendingOrder);
    QCOMPARE(m_model->sortRole(), QByteArray("name"));
    QCOMPARE(m_model->sortOrder(), Qt::DescendingOrder);
    QCOMPARE(itemsInModel(), QStringList() << "c" << "e" << "d" << "b" << "a");

    // Sort by Date, decending
    m_model->setSortRole("date");
    QCOMPARE(m_model->sortRole(), QByteArray("date"));
    QCOMPARE(m_model->sortOrder(), Qt::DescendingOrder);
    QCOMPARE(itemsInModel(), QStringList() << "c" << "b" << "d" << "a" << "e");

    // Sort by Date, ascending
    m_model->setSortOrder(Qt::AscendingOrder);
    QCOMPARE(m_model->sortRole(), QByteArray("date"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QCOMPARE(itemsInModel(), QStringList() << "c" << "e" << "a" << "d" << "b");

    // Sort by Date, ascending, 'Sort Folders First' disabled
    m_model->setSortFoldersFirst(false);
    QCOMPARE(m_model->sortRole(), QByteArray("date"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(!m_model->sortFoldersFirst());
    QCOMPARE(itemsInModel(), QStringList() << "e" << "a" << "c" << "d" << "b");

    // Default: Sort by Name, ascending, 'Sort Folders First' disabled
    m_model->setSortRole("name");
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(!m_model->sortFoldersFirst());
    QCOMPARE(itemsInModel(), QStringList() << "a" << "b" << "c" << "d" << "e");

    // Sort by Size, ascending, 'Sort Folders First' enabled
    m_model->setSortRole("size");
    m_model->setSortFoldersFirst(true);
    QCOMPARE(m_model->sortRole(), QByteArray("size"));
    QCOMPARE(m_model->sortOrder(), Qt::AscendingOrder);
    QVERIFY(m_model->sortFoldersFirst());
    QCOMPARE(itemsInModel(), QStringList() << "c" << "a" << "b" << "e" << "d");

    // Sort by Size, descending, 'Sort Folders First' enabled
    m_model->setSortOrder(Qt::DescendingOrder);
    QCOMPARE(m_model->sortRole(), QByteArray("size"));
    QCOMPARE(m_model->sortOrder(), Qt::DescendingOrder);
    QVERIFY(m_model->sortFoldersFirst());
    QCOMPARE(itemsInModel(), QStringList() << "c" << "d" << "e" << "b" << "a");

    // TODO: How shall the sorting by size be done if 'Sort Folders First' is disabled?

    // TODO: Sort by other roles; show/hide hidden files
}

void KFileItemModelTest::testExpansionLevelsCompare_data()
{
    QTest::addColumn<QString>("urlA");
    QTest::addColumn<QString>("urlB");
    QTest::addColumn<int>("result");

    QTest::newRow("Equal") << "/a/b" << "/a/b" << 0;
    QTest::newRow("Sub path: A < B") << "/a/b" << "/a/b/c" << -1;
    QTest::newRow("Sub path: A > B") << "/a/b/c" << "/a/b" << +1;
    QTest::newRow("Same level: /a/1 < /a-1/1") << "/a/1" << "/a-1/1" << -1;
    QTest::newRow("Same level: /a-/1 > /a/1") << "/a-1/1" << "/a/1" << +1;
    QTest::newRow("Different levels: /a/a/1 < /a/a-1") << "/a/a/1" << "/a/a-1" << -1;
    QTest::newRow("Different levels: /a/a-1 > /a/a/1") << "/a/a-1" << "/a/a/1" << +1;
}

void KFileItemModelTest::testExpansionLevelsCompare()
{
    QFETCH(QString, urlA);
    QFETCH(QString, urlB);
    QFETCH(int, result);

    const KFileItem a(KUrl(urlA), QString(), mode_t(-1));
    const KFileItem b(KUrl(urlB), QString(), mode_t(-1));
    QCOMPARE(m_model->expansionLevelsCompare(a, b), result);
}

void KFileItemModelTest::testIndexForKeyboardSearch()
{
    QStringList files;
    files << "a" << "aa" << "Image.jpg" << "Image.png" << "Text" << "Text1" << "Text2" << "Text11";
    m_testDir->createFiles(files);

    m_dirLister->openUrl(m_testDir->url());
    QVERIFY(QTest::kWaitForSignal(m_model, SIGNAL(itemsInserted(KItemRangeList)), DefaultTimeout));

    // Search from index 0
    QCOMPARE(m_model->indexForKeyboardSearch("a", 0), 0);
    QCOMPARE(m_model->indexForKeyboardSearch("aa", 0), 1);
    QCOMPARE(m_model->indexForKeyboardSearch("i", 0), 2);
    QCOMPARE(m_model->indexForKeyboardSearch("image", 0), 2);
    QCOMPARE(m_model->indexForKeyboardSearch("image.jpg", 0), 2);
    QCOMPARE(m_model->indexForKeyboardSearch("image.png", 0), 3);
    QCOMPARE(m_model->indexForKeyboardSearch("t", 0), 4);
    QCOMPARE(m_model->indexForKeyboardSearch("text", 0), 4);
    QCOMPARE(m_model->indexForKeyboardSearch("text1", 0), 5);
    QCOMPARE(m_model->indexForKeyboardSearch("text2", 0), 6);
    QCOMPARE(m_model->indexForKeyboardSearch("text11", 0), 7);

    // Start a search somewhere in the middle
    QCOMPARE(m_model->indexForKeyboardSearch("a", 1), 1);
    QCOMPARE(m_model->indexForKeyboardSearch("i", 3), 3);
    QCOMPARE(m_model->indexForKeyboardSearch("t", 5), 5);
    QCOMPARE(m_model->indexForKeyboardSearch("text1", 6), 7);

    // Test searches that go past the last item back to index 0
    QCOMPARE(m_model->indexForKeyboardSearch("a", 2), 0);
    QCOMPARE(m_model->indexForKeyboardSearch("i", 7), 2);
    QCOMPARE(m_model->indexForKeyboardSearch("image.jpg", 3), 2);
    QCOMPARE(m_model->indexForKeyboardSearch("text2", 7), 6);

    // Test searches that yield no result
    QCOMPARE(m_model->indexForKeyboardSearch("aaa", 0), -1);
    QCOMPARE(m_model->indexForKeyboardSearch("b", 0), -1);
    QCOMPARE(m_model->indexForKeyboardSearch("image.svg", 0), -1);
    QCOMPARE(m_model->indexForKeyboardSearch("text3", 0), -1);
    QCOMPARE(m_model->indexForKeyboardSearch("text3", 5), -1);

    // Test upper case searches (note that search is case insensitive)
    QCOMPARE(m_model->indexForKeyboardSearch("A", 0), 0);
    QCOMPARE(m_model->indexForKeyboardSearch("aA", 0), 1);
    QCOMPARE(m_model->indexForKeyboardSearch("TexT", 5), 5);
    QCOMPARE(m_model->indexForKeyboardSearch("IMAGE", 4), 2);

    // TODO: Maybe we should also test keyboard searches in directories which are not sorted by Name?
}

bool KFileItemModelTest::isModelConsistent() const
{
    for (int i = 0; i < m_model->count(); ++i) {
        const KFileItem item = m_model->fileItem(i);
        if (item.isNull()) {
            qWarning() << "Item" << i << "is null";
            return false;
        }

        const int itemIndex = m_model->index(item);
        if (itemIndex != i) {
            qWarning() << "Item" << i << "has a wrong index:" << itemIndex;
            return false;
        }
    }

    return true;
}

QStringList KFileItemModelTest::itemsInModel() const
{
    QStringList items;

    for (int i = 0; i < m_model->count(); i++) {
        items << m_model->data(i).value("name").toString();
    }

    return items;
}

QTEST_KDEMAIN(KFileItemModelTest, NoGUI)

#include "kfileitemmodeltest.moc"
