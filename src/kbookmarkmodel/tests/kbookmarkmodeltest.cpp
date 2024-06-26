/*
 *  SPDX-FileCopyrightText: 2010 David Faure <faure@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <KActionCollection>
#include <QAction>

#include <KBookmarkManager>
#include <QMimeData>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTest>

#include "../../kbookmarkmodel/commandhistory.h"
#include "../../kbookmarkmodel/commands.h" // TODO provide public API in the model instead?
#include "../../kbookmarkmodel/model.h"

// Return a list of all bookmark addresses or urls in a KBookmarkManager.
class BookmarkLister : public KBookmarkGroupTraverser
{
public:
    BookmarkLister(const KBookmarkGroup &root)
    {
        traverse(root);
    }
    static QStringList addressList(KBookmarkManager *mgr)
    {
        BookmarkLister lister(mgr->root());
        return lister.m_addressList;
    }
    static QStringList urlList(KBookmarkManager *mgr)
    {
        BookmarkLister lister(mgr->root());
        return lister.m_urlList;
    }
    static QStringList titleList(KBookmarkManager *mgr)
    {
        BookmarkLister lister(mgr->root());
        return lister.m_titleList;
    }
    void visit(const KBookmark &bk) override
    {
        m_addressList.append(bk.address());
        m_urlList.append(bk.url().url());
        m_titleList.append(bk.text());
    }
    void visitEnter(const KBookmarkGroup &group) override
    {
        m_addressList.append(group.address() + QLatin1Char('/'));
        m_titleList.append(group.text());
    }

private:
    QStringList m_addressList;
    QStringList m_urlList;
    QStringList m_titleList;
};

class KBookmarkModelTest : public QObject
{
    Q_OBJECT
public:
    KBookmarkModelTest()
        : m_collection(this)
    {
    }

private:
private Q_SLOTS:
    void initTestCase()
    {
        const QString filename = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/konqueror/bookmarks.xml");
        QFile::remove(filename);
        m_bookmarkManager = std::make_unique<KBookmarkManager>(filename);
        m_cmdHistory = new CommandHistory(this);
        m_cmdHistory->setBookmarkManager(m_bookmarkManager.get());
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()), QStringList());
        m_model = new KBookmarkModel(m_bookmarkManager->root(), m_cmdHistory, this);
        QCOMPARE(m_model->rowCount(), 1); // the toplevel "Bookmarks" toplevel item
        m_rootIndex = m_model->index(0, 0);
        QVERIFY(m_rootIndex.isValid());
        QCOMPARE(m_model->rowCount(m_rootIndex), 0);
        m_cmdHistory->createActions(&m_collection);
    }

    // The commands modify the model, so the test code uses the commands
    void testAddBookmark()
    {
        CreateCommand *cmd =
            new CreateCommand(m_model, QStringLiteral("/0"), QStringLiteral("test_bk"), QStringLiteral("www"), QUrl(QStringLiteral("https://www.kde.org")));
        cmd->redo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()), QStringList() << QStringLiteral("/0"));
        QCOMPARE(BookmarkLister::urlList(m_bookmarkManager.get()), QStringList() << QStringLiteral("https://www.kde.org"));
        QCOMPARE(m_model->rowCount(m_rootIndex), 1);
        cmd->undo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()), QStringList());
        QCOMPARE(m_model->rowCount(m_rootIndex), 0);
        delete cmd;
    }

    void testDeleteBookmark()
    {
        CreateCommand *cmd =
            new CreateCommand(m_model, QStringLiteral("/0"), QStringLiteral("test_bk"), QStringLiteral("www"), QUrl(QStringLiteral("https://www.kde.org")));
        cmd->redo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()), QStringList() << QStringLiteral("/0"));
        DeleteCommand *deleteCmd = new DeleteCommand(m_model, QStringLiteral("/0"));
        deleteCmd->redo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()), QStringList());
        deleteCmd->undo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()), QStringList() << QStringLiteral("/0"));
        deleteCmd->redo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()), QStringList());

        delete cmd;
        delete deleteCmd;
    }

    void testCreateFolder() // and test moving stuff around
    {
        CreateCommand *folderCmd = new CreateCommand(m_model, QStringLiteral("/0"), QStringLiteral("folder"), QStringLiteral("folder"), true /*open*/);
        m_cmdHistory->addCommand(folderCmd); // calls redo
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()), QStringList() << QStringLiteral("/0/"));
        QCOMPARE(m_model->rowCount(m_rootIndex), 1);

        const QString kde = QStringLiteral("https://www.kde.org");
        CreateCommand *cmd = new CreateCommand(m_model, QStringLiteral("/0/0"), QStringLiteral("test_bk"), QStringLiteral("www"), QUrl(kde));
        m_cmdHistory->addCommand(cmd); // calls redo
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()), QStringList() << QStringLiteral("/0/") << QStringLiteral("/0/0"));

        // Insert before this bookmark
        const QString first = QStringLiteral("https://first.example.com");
        m_cmdHistory->addCommand(new CreateCommand(m_model, QStringLiteral("/0/0"), QStringLiteral("first_bk"), QStringLiteral("www"), QUrl(first)));
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()),
                 QStringList() << QStringLiteral("/0/") << QStringLiteral("/0/0") << QStringLiteral("/0/1"));
        QCOMPARE(BookmarkLister::urlList(m_bookmarkManager.get()), QStringList() << first << kde);

        // Move the kde bookmark before the first bookmark
        KBookmark kdeBookmark = m_bookmarkManager->findByAddress(QStringLiteral("/0/1"));
        QCOMPARE(kdeBookmark.url().url(), kde);
        QModelIndex kdeIndex = m_model->indexForBookmark(kdeBookmark);
        QCOMPARE(kdeIndex.row(), 1);
        QCOMPARE(m_model->rowCount(kdeIndex.parent()), 2);

        QMimeData *mimeData = m_model->mimeData(QModelIndexList() << kdeIndex);
        bool ok = m_model->dropMimeData(mimeData, Qt::MoveAction, 0, 0, kdeIndex.parent());
        QVERIFY(ok);
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()),
                 QStringList() << QStringLiteral("/0/") << QStringLiteral("/0/0") << QStringLiteral("/0/1"));
        QCOMPARE(BookmarkLister::urlList(m_bookmarkManager.get()), QStringList() << kde << first);
        delete mimeData;

        // Move the kde bookmark after the bookmark called 'first'
        kdeBookmark = m_bookmarkManager->findByAddress(QStringLiteral("/0/0"));
        kdeIndex = m_model->indexForBookmark(kdeBookmark);
        QCOMPARE(kdeIndex.row(), 0);
        mimeData = m_model->mimeData(QModelIndexList() << kdeIndex);
        ok = m_model->dropMimeData(mimeData, Qt::MoveAction, 2, 0, kdeIndex.parent());
        QVERIFY(ok);
        QCOMPARE(BookmarkLister::urlList(m_bookmarkManager.get()), QStringList() << first << kde);
        delete mimeData;

        // Create new folder, then move both bookmarks into it (#287038)
        m_cmdHistory->addCommand(new CreateCommand(m_model, QStringLiteral("/1"), QStringLiteral("folder2"), QStringLiteral("folder2"), true));
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()),
                 QStringList() << QStringLiteral("/0/") << QStringLiteral("/0/0") << QStringLiteral("/0/1") << QStringLiteral("/1/"));
        QCOMPARE(m_model->rowCount(m_rootIndex), 2);

        moveTwoBookmarks(QStringLiteral("/0/0"), QStringLiteral("/0/1"), QStringLiteral("/1"));
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()),
                 QStringList() << QStringLiteral("/0/") << QStringLiteral("/1/") << QStringLiteral("/1/0") << QStringLiteral("/1/1"));
        QCOMPARE(BookmarkLister::urlList(m_bookmarkManager.get()), QStringList() << kde << first);

        // Move bookmarks from /1 into subfolder /1/2 (which will become /1/0)
        m_cmdHistory->addCommand(new CreateCommand(m_model, QStringLiteral("/1/2"), QStringLiteral("subfolder"), QStringLiteral("subfolder"), true));
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()),
                 QStringList() << QStringLiteral("/0/") << QStringLiteral("/1/") << QStringLiteral("/1/0") << QStringLiteral("/1/1")
                               << QStringLiteral("/1/2/"));
        moveTwoBookmarks(QStringLiteral("/1/0"), QStringLiteral("/1/1"), QStringLiteral("/1/2"));
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()),
                 QStringList() << QStringLiteral("/0/") << QStringLiteral("/1/") << QStringLiteral("/1/0/") << QStringLiteral("/1/0/0")
                               << QStringLiteral("/1/0/1"));

        // Move them up again
        moveTwoBookmarks(QStringLiteral("/1/0/0"), QStringLiteral("/1/0/1"), QStringLiteral("/1"));
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()),
                 QStringList() << QStringLiteral("/0/") << QStringLiteral("/1/") << QStringLiteral("/1/0") << QStringLiteral("/1/1")
                               << QStringLiteral("/1/2/"));

        undoAll();
    }

    void testSort()
    {
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()), QStringList());
        CreateCommand *folderCmd = new CreateCommand(m_model, QStringLiteral("/0"), QStringLiteral("folder"), QStringLiteral("folder"), true /*open*/);
        m_cmdHistory->addCommand(folderCmd); // calls redo
        const QString kde = QStringLiteral("https://www.kde.org");
        QStringList bookmarks;
        bookmarks << QStringLiteral("Faure") << QStringLiteral("Web") << QStringLiteral("Kde") << QStringLiteral("Avatar") << QStringLiteral("David");
        for (int i = 0; i < bookmarks.count(); ++i) {
            m_cmdHistory->addCommand(new CreateCommand(m_model, QStringLiteral("/0/") + QString::number(i), bookmarks[i], QStringLiteral("www"), QUrl(kde)));
        }
        const QStringList addresses = BookmarkLister::addressList(m_bookmarkManager.get());
        // qCDebug(KEDITBOOKMARKS_LOG) << addresses;
        const QStringList origTitleList = BookmarkLister::titleList(m_bookmarkManager.get());
        QCOMPARE(addresses.count(), bookmarks.count() + 1 /* parent folder */);
        SortCommand *sortCmd = new SortCommand(m_model, QStringLiteral("Sort"), QStringLiteral("/0"));
        m_cmdHistory->addCommand(sortCmd);
        QStringList expectedTitleList = bookmarks;
        expectedTitleList.sort();
        expectedTitleList.prepend(QStringLiteral("folder"));
        const QStringList sortedTitles = BookmarkLister::titleList(m_bookmarkManager.get());
        // qCDebug(KEDITBOOKMARKS_LOG) << sortedTitles;
        QCOMPARE(sortedTitles, expectedTitleList);

        sortCmd->undo();
        QCOMPARE(BookmarkLister::titleList(m_bookmarkManager.get()), origTitleList);
        sortCmd->redo();
        undoAll();
    }

    void testSortBug258505()
    {
        // Given a specific set of bookmarks from XML
        const QString testFile(QFINDTESTDATA("kde-bug-258505-bookmarks.xml"));
        QVERIFY(!testFile.isEmpty());
        //  (copied to a temp file to avoid saving the sorted bookmarks)
        QFile xmlFile(testFile);
        QVERIFY(xmlFile.open(QIODevice::ReadOnly));
        const QByteArray data = xmlFile.readAll();
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        const QString fileName = tempFile.fileName();
        tempFile.write(data);
        tempFile.close();
        KBookmarkManager bookmarkManager(fileName);

        const QStringList addresses = BookmarkLister::addressList(&bookmarkManager);
        const QStringList origTitleList = BookmarkLister::titleList(&bookmarkManager);
        // qCDebug(KEDITBOOKMARKS_LOG) << addresses << origTitleList;
        QCOMPARE(addresses.count(), 53);

        CommandHistory cmdHistory;
        cmdHistory.setBookmarkManager(&bookmarkManager);
        KBookmarkModel *model = new KBookmarkModel(bookmarkManager.root(), &cmdHistory, this);
        QCOMPARE(model->rowCount(), 1); // the toplevel "Bookmarks" toplevel item

        // When sorting
        SortCommand *sortCmd = new SortCommand(model, QStringLiteral("Sort"), QStringLiteral("/0"));
        cmdHistory.addCommand(sortCmd);

        // Then the contents should be correctly sorted
        const QStringList sortedTitles = BookmarkLister::titleList(&bookmarkManager);
        QCOMPARE(sortedTitles.at(0), QStringLiteral("Cyclone V"));
        QCOMPARE(sortedTitles.at(1), QStringLiteral("Altera SoC design courses"));
        QCOMPARE(sortedTitles.at(2), QStringLiteral("Hardware Design Fl...r an ARM-based SoC"));
        // ...

        // And when undoing
        sortCmd->undo();

        // Then the contents should revert to the orig order
        QCOMPARE(BookmarkLister::titleList(&bookmarkManager), origTitleList);

        QFile::remove(fileName + QLatin1String(".bak"));
    }

private:
    void moveTwoBookmarks(const QString &src1, const QString &src2, const QString &dest)
    {
        const QModelIndex firstIndex = m_model->indexForBookmark(m_bookmarkManager->findByAddress(src1));
        const QModelIndex secondIndex = m_model->indexForBookmark(m_bookmarkManager->findByAddress(src2));
        QMimeData *mimeData = m_model->mimeData(QModelIndexList() << firstIndex << secondIndex);
        QModelIndex folder2Index = m_model->indexForBookmark(m_bookmarkManager->findByAddress(dest));
        QVERIFY(m_model->dropMimeData(mimeData, Qt::MoveAction, -1, 0, folder2Index));
        delete mimeData;
    }

    void undoAll()
    {
        QAction *undoAction = m_collection.action(KStandardAction::name(KStandardAction::Undo));
        QVERIFY(undoAction);
        while (undoAction->isEnabled()) {
            undoAction->trigger();
        }
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager.get()), QStringList());
    }

    std::unique_ptr<KBookmarkManager> m_bookmarkManager;
    KBookmarkModel *m_model;
    CommandHistory *m_cmdHistory;
    KActionCollection m_collection;
    QModelIndex m_rootIndex; // the index of the "Bookmarks" root
};

QTEST_MAIN(KBookmarkModelTest)

#include "kbookmarkmodeltest.moc"
