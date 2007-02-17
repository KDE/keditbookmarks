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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "bookmarkselector.h"

#include <assert.h>
#include <q3popupmenu.h>
#include <qpainter.h>
#include <qpixmap.h>

#include <kiconloader.h>
#include <kglobalsettings.h>
#include <kbookmarkmanager.h>

#include "dolphinsettings.h"
#include "urlnavigator.h"

BookmarkSelector::BookmarkSelector(UrlNavigator* parent) :
    UrlButton(parent),
    m_selectedIndex(0),
    m_urlNavigator(parent)
{
    setFocusPolicy(Qt::NoFocus);

    m_bookmarksMenu = new Q3PopupMenu(this);

    KBookmarkGroup root = DolphinSettings::instance().bookmarkManager()->root();
    KBookmark bookmark = root.first();
    int i = 0;
    while (!bookmark.isNull()) {
        m_bookmarksMenu->insertItem(MainBarIcon(bookmark.icon()),
                                    bookmark.text(),
                                    i);
        if (i == m_selectedIndex) {
            QPixmap pixmap = SmallIcon(bookmark.icon());
            setIcon(QIcon(pixmap));
            setIconSize(pixmap.size());
            setMinimumWidth(pixmap.width() + 2);
        }
        bookmark = root.next(bookmark);
        ++i;
    }

    connect(m_bookmarksMenu, SIGNAL(activated(int)),
            this, SLOT(slotBookmarkActivated(int)));

    setMenu(m_bookmarksMenu);
}

BookmarkSelector::~BookmarkSelector()
{
}

void BookmarkSelector::updateSelection(const KUrl& url)
{
    KBookmarkGroup root = DolphinSettings::instance().bookmarkManager()->root();
    KBookmark bookmark = root.first();

    int maxLength = 0;
    m_selectedIndex = -1;

    // Search the bookmark which is equal to the Url or at least is a parent Url.
    // If there are more than one possible parent Url candidates, choose the bookmark
    // which covers the bigger range of the Url.
    int i = 0;
    while (!bookmark.isNull()) {
        const KUrl bookmarkUrl = bookmark.url();
        if (bookmarkUrl.isParentOf(url)) {
            const int length = bookmarkUrl.prettyUrl().length();
            if (length > maxLength) {
                m_selectedIndex = i;
                setIcon(SmallIcon(bookmark.icon()));
                maxLength = length;
            }
        }
        bookmark = root.next(bookmark);
        ++i;
    }

    if (m_selectedIndex < 0) {
        // No bookmark has been found which matches to the given Url. Show
        // a generic folder icon as pixmap for indication:
        setIcon(SmallIcon("folder"));
    }
}

KBookmark BookmarkSelector::selectedBookmark() const
{
    return DolphinSettings::instance().bookmark(m_selectedIndex);
}

QSize BookmarkSelector::sizeHint() const
{
    const int height = UrlButton::sizeHint().height();
    return QSize(height, height);
}

void BookmarkSelector::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    const int buttonWidth  = width();
    const int buttonHeight = height();

    QColor backgroundColor;
    QColor foregroundColor;
    const bool isHighlighted = isDisplayHintEnabled(EnteredHint) ||
                               isDisplayHintEnabled(DraggedHint);
    if (isHighlighted) {
        backgroundColor = KGlobalSettings::highlightColor();
        foregroundColor = KGlobalSettings::highlightedTextColor();
    }
    else {
        backgroundColor = palette().brush(QPalette::Background).color();
        foregroundColor = KGlobalSettings::buttonTextColor();
    }

    // dimm the colors if the parent view does not have the focus
    const bool isActive = m_urlNavigator->isActive();
    if (!isActive) {
        QColor dimmColor(palette().brush(QPalette::Background).color());
        foregroundColor = mixColors(foregroundColor, dimmColor);
        if (isHighlighted) {
            backgroundColor = mixColors(backgroundColor, dimmColor);
        }
    }

    if (!(isDisplayHintEnabled(ActivatedHint) && isActive) && !isHighlighted) {
        // dimm the foreground color by mixing it with the background
        foregroundColor = mixColors(foregroundColor, backgroundColor);
        painter.setPen(foregroundColor);
    }

    // draw button backround
    painter.setPen(Qt::NoPen);
    painter.setBrush(backgroundColor);
    painter.drawRect(0, 0, buttonWidth, buttonHeight);

    // draw icon
    const QPixmap pixmap = icon().pixmap();
    const int x = (buttonWidth -  pixmap.width()) / 2;
    const int y = (buttonHeight - pixmap.height()) / 2;
    painter.drawPixmap(x, y, pixmap);
}

void BookmarkSelector::slotBookmarkActivated(int index)
{
    m_selectedIndex = index;

    const KBookmark bookmark = selectedBookmark();
    setPixmap(SmallIcon(bookmark.icon()));
    emit bookmarkActivated(bookmark.url());
}

#include "bookmarkselector.moc"

