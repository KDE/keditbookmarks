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

#ifndef TESTHELPER_H
#define TESTHELPER_H

#include <KUrl>

class KTempDir;
class QAbstractItemView;
class QDir;
class DolphinDirLister;
class DolphinModel;
class DolphinSortFilterProxyModel;
class DolphinView;

/*
 * The class TestHelper aims to make writing Dolphin unit tests easier.
 * It provides functionality that almost every unit test needs: setup of the DolphinView and
 * easy creation of test files and subfolders in a temporary directory which is removed in
 * the TestHelper destructor.
 *
 * TODO: TestHelper should also backup the DolphinSettings and restore them later!
 */

class TestHelper
{

public:

    TestHelper();
    ~TestHelper();

    DolphinView* view() const { return m_view; }
    DolphinSortFilterProxyModel* proxyModel() const { return m_proxyModel; }

    // Returns the item view (icons, details, or columns)
    QAbstractItemView* itemView () const;

    KUrl testDirUrl() const;

    /*
     * The following functions create either a file, a list of files, or a directory.
     * The paths may be absolute or relative to the test directory. Any missing parent
     * directories will be created automatically.
     */

    void createFile(const QString& path, const QByteArray& data = QByteArray("test"));
    void createFiles(const QStringList& files);
    void createDir(const QString& path);

    /*
     * Remove the test directory and create an empty one.
     */

    void cleanupTestDir();

private:

    void makePathAbsoluteAndCreateParents(QString& path);

    KTempDir* m_tempDir;
    QString m_path;
    QDir* m_dir;
    DolphinDirLister* m_dirLister;
    DolphinModel* m_dolphinModel;
    DolphinSortFilterProxyModel* m_proxyModel;
    DolphinView* m_view;

};

#endif