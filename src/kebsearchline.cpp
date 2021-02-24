/* This file is part of the KDE project
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kebsearchline.h"
#include "keditbookmarks_debug.h"
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QTimer>
#include <QTreeView>

#include <KLocalizedString>

#include <QHeaderView>
#include <QMenu>

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////
class KViewSearchLine::KViewSearchLinePrivate
{
public:
    KViewSearchLinePrivate()
        : listView(nullptr)
        , treeView(nullptr)
        , caseSensitive(false)
        , activeSearch(false)
        , keepParentsVisible(true)
        , queuedSearches(0)
    {
    }

    QListView *listView;
    QTreeView *treeView;
    bool caseSensitive;
    bool activeSearch;
    bool keepParentsVisible;
    QString search;
    int queuedSearches;
    QVector<int> searchColumns;
};

KViewSearchLine::KViewSearchLine(QWidget *parent, QAbstractItemView *v)
    : KLineEdit(parent)
{
    d = new KViewSearchLinePrivate;

    setClearButtonEnabled(true);

    d->treeView = dynamic_cast<QTreeView *>(v);
    d->listView = dynamic_cast<QListView *>(v);

    connect(this, &KViewSearchLine::textChanged, this, &KViewSearchLine::queueSearch);

    if (view()) {
        connect(view(), &QObject::destroyed, this, &KViewSearchLine::listViewDeleted);
        connect(model(), &QAbstractItemModel::dataChanged, this, &KViewSearchLine::slotDataChanged);
        connect(model(), &QAbstractItemModel::rowsInserted, this, &KViewSearchLine::slotRowsInserted);
        connect(model(), &QAbstractItemModel::rowsRemoved, this, &KViewSearchLine::slotRowsRemoved);
        connect(model(), &QAbstractItemModel::columnsInserted, this, &KViewSearchLine::slotColumnsInserted);
        connect(model(), &QAbstractItemModel::columnsRemoved, this, &KViewSearchLine::slotColumnsRemoved);
        connect(model(), &QAbstractItemModel::modelReset, this, &KViewSearchLine::slotModelReset);
    } else
        setEnabled(false);
}

KViewSearchLine::KViewSearchLine(QWidget *parent)
    : KLineEdit(parent)
{
    d = new KViewSearchLinePrivate;

    setClearButtonEnabled(true);

    d->treeView = nullptr;
    d->listView = nullptr;

    connect(this, &KViewSearchLine::textChanged, this, &KViewSearchLine::queueSearch);

    setEnabled(false);
}

KViewSearchLine::~KViewSearchLine()
{
    delete d;
}

QAbstractItemView *KViewSearchLine::view() const
{
    if (d->treeView)
        return d->treeView;
    else
        return d->listView;
}

bool KViewSearchLine::caseSensitive() const
{
    return d->caseSensitive;
}

bool KViewSearchLine::keepParentsVisible() const
{
    return d->keepParentsVisible;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void KViewSearchLine::updateSearch(const QString &s)
{
    if (!view())
        return;

    d->search = s.isNull() ? text() : s;

    // If there's a selected item that is visible, make sure that it's visible
    // when the search changes too (assuming that it still matches).
    // FIXME reimplement

    if (d->keepParentsVisible)
        checkItemParentsVisible(model()->index(0, 0, QModelIndex()));
    else
        checkItemParentsNotVisible();
}

void KViewSearchLine::setCaseSensitive(bool cs)
{
    d->caseSensitive = cs;
}

void KViewSearchLine::setKeepParentsVisible(bool v)
{
    d->keepParentsVisible = v;
}

void KViewSearchLine::setSearchColumns(const QVector<int> &columns)
{
    d->searchColumns = columns;
}

void KViewSearchLine::setView(QAbstractItemView *v)
{
    if (view()) {
        disconnect(view(), &QObject::destroyed, this, &KViewSearchLine::listViewDeleted);
        disconnect(model(), &QAbstractItemModel::dataChanged, this, &KViewSearchLine::slotDataChanged);
        disconnect(model(), &QAbstractItemModel::rowsInserted, this, &KViewSearchLine::slotRowsInserted);
        disconnect(model(), &QAbstractItemModel::rowsRemoved, this, &KViewSearchLine::slotRowsRemoved);
        disconnect(model(), &QAbstractItemModel::columnsInserted, this, &KViewSearchLine::slotColumnsInserted);
        disconnect(model(), &QAbstractItemModel::columnsRemoved, this, &KViewSearchLine::slotColumnsRemoved);
        disconnect(model(), &QAbstractItemModel::modelReset, this, &KViewSearchLine::slotModelReset);
    }

    d->treeView = dynamic_cast<QTreeView *>(v);
    d->listView = dynamic_cast<QListView *>(v);

    if (view()) {
        connect(view(), &QObject::destroyed, this, &KViewSearchLine::listViewDeleted);

        connect(model(), &QAbstractItemModel::dataChanged, this, &KViewSearchLine::slotDataChanged);
        connect(model(), &QAbstractItemModel::rowsInserted, this, &KViewSearchLine::slotRowsInserted);
        connect(model(), &QAbstractItemModel::rowsRemoved, this, &KViewSearchLine::slotRowsRemoved);
        connect(model(), &QAbstractItemModel::columnsInserted, this, &KViewSearchLine::slotColumnsInserted);
        connect(model(), &QAbstractItemModel::columnsRemoved, this, &KViewSearchLine::slotColumnsRemoved);
        connect(model(), &QAbstractItemModel::modelReset, this, &KViewSearchLine::slotModelReset);
    }

    setEnabled(bool(view()));
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

bool KViewSearchLine::itemMatches(const QModelIndex &item, const QString &s) const
{
    if (s.isEmpty())
        return true;

    // If the search column list is populated, search just the columns
    // specified.  If it is empty default to searching all of the columns.
    if (d->treeView) {
        int columnCount = d->treeView->header()->count();
        int row = item.row();
        QModelIndex parent = item.parent();
        if (!d->searchColumns.isEmpty()) {
            QVector<int>::const_iterator it, end;
            end = d->searchColumns.constEnd();
            for (it = d->searchColumns.constBegin(); it != end; ++it) {
                if (*it < columnCount) {
                    const QString &text = model()->data(model()->index(row, *it, parent)).toString();
                    if (text.indexOf(s, 0, d->caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive) >= 0)
                        return true;
                }
            }
        } else {
            for (int i = 0; i < columnCount; i++) {
                if (d->treeView->isColumnHidden(i) == false) {
                    const QString &text = model()->data(model()->index(row, i, parent)).toString();
                    if (text.indexOf(s, 0, d->caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive) >= 0)
                        return true;
                }
            }
        }
        return false;
    } else {
        QString text = model()->data(item).toString();
        if (text.indexOf(s, 0, d->caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive) >= 0)
            return true;
        else
            return false;
    }
}

void KViewSearchLine::contextMenuEvent(QContextMenuEvent *e)
{
    qDeleteAll(actions);
    QMenu *popup = KLineEdit::createStandardContextMenu();
    if (d->treeView) {
        int columnCount = d->treeView->header()->count();
        actions.resize(columnCount + 1);
        if (columnCount) {
            QMenu *submenu = new QMenu(i18n("Search Columns"), popup);
            popup->addMenu(submenu);
            bool allVisibleColumsCheked = true;
            QAction *allVisibleAct = new QAction(i18n("All Visible Columns"), nullptr);
            allVisibleAct->setCheckable(true);
            submenu->addAction(allVisibleAct);
            submenu->addSeparator();
            for (int i = 0; i < columnCount; ++i) {
                int logicalIndex = d->treeView->header()->logicalIndex(i);
                QString columnText = model()->headerData(logicalIndex, Qt::Horizontal).toString();
                if (columnText.isEmpty())
                    columnText = i18nc("Column number %1", "Column No. %1", i);
                QAction *act = new QAction(columnText, nullptr);
                act->setCheckable(true);
                if (d->searchColumns.isEmpty() || d->searchColumns.contains(logicalIndex))
                    act->setChecked(true);

                actions[logicalIndex] = act;
                if (!d->treeView || (d->treeView->isColumnHidden(i) == false)) {
                    submenu->addAction(act);
                    allVisibleColumsCheked = allVisibleColumsCheked && act->isChecked();
                }
            }
            actions[columnCount] = allVisibleAct;
            if (d->searchColumns.isEmpty() || allVisibleColumsCheked) {
                allVisibleAct->setChecked(true);
                d->searchColumns.clear();
            }
            connect(submenu, &QMenu::triggered, this, &KViewSearchLine::searchColumnsMenuActivated);
        }
    }
    popup->exec(e->globalPos());
    delete popup;
}

////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

void KViewSearchLine::queueSearch(const QString &search)
{
    d->queuedSearches++;
    d->search = search;
    QTimer::singleShot(200, this, &KViewSearchLine::activateSearch);
}

void KViewSearchLine::activateSearch()
{
    --(d->queuedSearches);

    if (d->queuedSearches == 0)
        updateSearch(d->search);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void KViewSearchLine::listViewDeleted()
{
    d->treeView = nullptr;
    d->listView = nullptr;
    setEnabled(false);
}

void KViewSearchLine::searchColumnsMenuActivated(QAction *action)
{
    int index = 0;
    int count = actions.count();
    for (int i = 0; i < count; ++i) {
        if (action == actions[i]) {
            index = i;
            break;
        }
    }
    int columnCount = d->treeView->header()->count();
    if (index == columnCount) {
        if (d->searchColumns.isEmpty()) // all columns was checked
            d->searchColumns.append(0);
        else
            d->searchColumns.clear();
    } else {
        if (d->searchColumns.contains(index))
            d->searchColumns.removeAll(index);
        else {
            if (d->searchColumns.isEmpty()) // all columns was checked
            {
                for (int i = 0; i < columnCount; ++i)
                    if (i != index)
                        d->searchColumns.append(i);
            } else
                d->searchColumns.append(index);
        }
    }
    updateSearch();
}

void KViewSearchLine::slotRowsRemoved(const QModelIndex &parent, int, int)
{
    if (!d->keepParentsVisible)
        return;

    QModelIndex p = parent;
    while (p.isValid()) {
        int count = model()->rowCount(p);
        if (count && anyVisible(model()->index(0, 0, p), model()->index(count - 1, 0, p)))
            return;
        if (itemMatches(p, d->search))
            return;
        setVisible(p, false);
        p = p.parent();
    }
}

void KViewSearchLine::slotColumnsInserted(const QModelIndex &, int, int)
{
    updateSearch();
}

void KViewSearchLine::slotColumnsRemoved(const QModelIndex &, int first, int last)
{
    if (d->treeView)
        updateSearch();
    else {
        if (d->listView->modelColumn() >= first && d->listView->modelColumn() <= last) {
            if (d->listView->modelColumn() > last)
                qCCritical(KEDITBOOKMARKS_LOG) << "Columns were removed, the modelColumn() doesn't exist anymore. "
                                                  "K4listViewSearchLine can't cope with that.";
            updateSearch();
        }
    }
}

void KViewSearchLine::slotModelReset()
{
    // FIXME Is there a way to ensure that the view
    // has already responded to the reset signal?
    updateSearch();
}

void KViewSearchLine::slotDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QModelIndex parent = topLeft.parent();
    int column = 0;
    if (d->listView)
        column = d->listView->modelColumn();
    bool match = recheck(model()->index(topLeft.row(), column, parent), model()->index(bottomRight.row(), column, parent));
    if (!d->keepParentsVisible)
        return;
    if (!parent.isValid()) // includes listview
        return;
    if (match) {
        QModelIndex p = parent;
        while (p.isValid()) {
            setVisible(p, true);
            p = p.parent();
        }
    } else // no match => might need to hide parents (this is ugly)
    {
        if (isVisible(parent) == false) // parent is already hidden
            return;
        // parent is visible => implies all parents visible

        // first check if all of the unchanged rows are hidden
        match = false;
        if (topLeft.row() >= 1)
            match = match || anyVisible(model()->index(0, 0, parent), model()->index(topLeft.row() - 1, 0, parent));
        int rowCount = model()->rowCount(parent);
        if (bottomRight.row() + 1 <= rowCount - 1)
            match = match || anyVisible(model()->index(bottomRight.row() + 1, 0, parent), model()->index(rowCount - 1, 0, parent));
        if (!match) // all child rows hidden
        {
            if (itemMatches(parent, d->search))
                return;
            // and parent didn't match, hide it
            setVisible(parent, false);

            // need to check all the way up to root
            QModelIndex p = parent.parent();
            while (p.isValid()) {
                // hide p if no children of p isVisible and it doesn't match
                int count = model()->rowCount(p);
                if (anyVisible(model()->index(0, 0, p), model()->index(count - 1, 0, p)))
                    return;

                if (itemMatches(p, d->search))
                    return;
                setVisible(p, false);
                p = p.parent();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////
QAbstractItemModel *KViewSearchLine::model() const
{
    if (d->treeView)
        return d->treeView->model();
    else
        return d->listView->model();
}

bool KViewSearchLine::anyVisible(const QModelIndex &first, const QModelIndex &last)
{
    Q_ASSERT(d->treeView);
    QModelIndex index = first;
    while (true) {
        if (isVisible(index))
            return true;
        if (index == last)
            break;
        index = nextRow(index);
    }
    return false;
}

bool KViewSearchLine::isVisible(const QModelIndex &index)
{
    if (d->treeView)
        return !d->treeView->isRowHidden(index.row(), index.parent());
    else
        return d->listView->isRowHidden(index.row());
}

QModelIndex KViewSearchLine::nextRow(const QModelIndex &index)
{
    return model()->index(index.row() + 1, index.column(), index.parent());
}

bool KViewSearchLine::recheck(const QModelIndex &first, const QModelIndex &last)
{
    bool visible = false;
    QModelIndex index = first;
    while (true) {
        int rowCount = model()->rowCount(index);
        if (d->keepParentsVisible && rowCount && anyVisible(model()->index(0, 0, index), model()->index(rowCount - 1, 0, index))) {
            visible = true;
        } else // no children visible
        {
            bool match = itemMatches(index, d->search);
            setVisible(index, match);
            visible = visible || match;
        }
        if (index == last)
            break;
        index = nextRow(index);
    }
    return visible;
}

void KViewSearchLine::slotRowsInserted(const QModelIndex &parent, int first, int last)
{
    bool visible = false;
    int column = 0;
    if (d->listView)
        column = d->listView->modelColumn();

    QModelIndex index = model()->index(first, column, parent);
    QModelIndex end = model()->index(last, column, parent);
    while (true) {
        if (itemMatches(index, d->search)) {
            visible = true;
            setVisible(index, true);
        } else
            setVisible(index, false);
        if (index == end)
            break;
        index = nextRow(index);
    }

    if (!d->keepParentsVisible)
        return;
    if (visible) {
        QModelIndex p = parent;
        while (p.isValid()) {
            setVisible(p, true);
            p = p.parent();
        }
    }
}

void KViewSearchLine::setVisible(const QModelIndex &index, bool v)
{
    if (d->treeView)
        d->treeView->setRowHidden(index.row(), index.parent(), !v);
    else
        d->listView->setRowHidden(index.row(), !v);
}

void KViewSearchLine::checkItemParentsNotVisible()
{
    int rowCount = model()->rowCount(QModelIndex());
    int column = 0;
    if (d->listView)
        column = d->listView->modelColumn();
    for (int i = 0; i < rowCount; ++i) {
        QModelIndex it = model()->index(i, column, QModelIndex());
        if (itemMatches(it, d->search))
            setVisible(it, true);
        else
            setVisible(it, false);
    }
}

/** Check whether \p index, its siblings and their descendants should be shown. Show or hide the items as necessary.
 *
 *  \p index  The list view item to start showing / hiding items at. Typically, this is the first child of another item, or the
 *              the first child of the list view.
 *  \return \c true if an item which should be visible is found, \c false if all items found should be hidden.
 */
bool KViewSearchLine::checkItemParentsVisible(QModelIndex index)
{
    bool visible = false;
    int rowCount = model()->rowCount(index.parent());
    int column = 0;
    if (d->listView)
        column = d->listView->modelColumn();
    for (int i = 0; i < rowCount; ++i) {
        index = model()->index(i, column, index.parent());
        if ((model()->rowCount(index) && checkItemParentsVisible(model()->index(0, column, index))) || itemMatches(index, d->search)) {
            visible = true;
            setVisible(index, true);
        } else
            setVisible(index, false);
    }
    return visible;
}

////////////////////////////////////////////////////////////////////////////////
// KViewSearchLineWidget
////////////////////////////////////////////////////////////////////////////////

class KViewSearchLineWidget::KViewSearchLineWidgetPrivate
{
public:
    KViewSearchLineWidgetPrivate()
        : view(nullptr)
        , searchLine(nullptr)
        , layout(nullptr)
    {
    }
    QAbstractItemView *view;
    KViewSearchLine *searchLine;
    QHBoxLayout *layout;
};

KViewSearchLineWidget::KViewSearchLineWidget(QAbstractItemView *view, QWidget *parent)
    : QWidget(parent)
{
    d = new KViewSearchLineWidgetPrivate;
    d->view = view;

    QTimer::singleShot(0, this, &KViewSearchLineWidget::createWidgets);
}

KViewSearchLineWidget::~KViewSearchLineWidget()
{
    delete d->layout;
    delete d;
}

KViewSearchLine *KViewSearchLineWidget::createSearchLine(QAbstractItemView *view)
{
    if (!d->searchLine)
        d->searchLine = new KViewSearchLine(nullptr, view);
    return d->searchLine;
}

void KViewSearchLineWidget::createWidgets()
{
    d->layout = new QHBoxLayout(this);
    d->layout->setContentsMargins(0, 0, 0, 0);

    QLabel *label = new QLabel(i18n("S&earch:"));
    label->setObjectName(QStringLiteral("kde toolbar widget"));
    d->layout->addWidget(label);

    d->searchLine = createSearchLine(d->view);
    d->layout->addWidget(d->searchLine);
    d->searchLine->show();

    label->setBuddy(d->searchLine);
    label->show();
}

KViewSearchLine *KViewSearchLineWidget::searchLine() const
{
    return d->searchLine;
}
