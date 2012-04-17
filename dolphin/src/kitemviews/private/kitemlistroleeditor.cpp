/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kitemlistroleeditor.h"

#include <KDebug>
#include <QKeyEvent>

KItemListRoleEditor::KItemListRoleEditor(QWidget *parent) :
    KTextEdit(parent),
    m_index(0),
    m_role()
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    document()->setDocumentMargin(0);

    if (parent) {
        parent->installEventFilter(this);
    }
}

KItemListRoleEditor::~KItemListRoleEditor()
{
}

void KItemListRoleEditor::setIndex(int index)
{
    m_index = index;
}

int KItemListRoleEditor::index() const
{
    return m_index;
}

void KItemListRoleEditor::setRole(const QByteArray& role)
{
    m_role = role;
}

QByteArray KItemListRoleEditor::role() const
{
    return m_role;
}

bool  KItemListRoleEditor::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        autoAdjustSize();
    }

    return KTextEdit::eventFilter(watched, event);
}

bool KItemListRoleEditor::event(QEvent* event)
{
    if (event->type() == QEvent::FocusOut) {
        emit roleEditingFinished(m_index, m_role, toPlainText());
    }
    return KTextEdit::event(event);
}

void KItemListRoleEditor::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        emit roleEditingCanceled(m_index, m_role, toPlainText());
        event->accept();
        return;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        emit roleEditingFinished(m_index, m_role, toPlainText());
        event->accept();
        return;
    default:
        break;
    }

    KTextEdit::keyPressEvent(event);
    autoAdjustSize();
}

void KItemListRoleEditor::autoAdjustSize()
{
    const qreal requiredWidth = document()->size().width();
    const qreal availableWidth = size().width() - 2 * frameWidth();
    if (requiredWidth > availableWidth) {
        qreal newWidth = requiredWidth + 2 * frameWidth();
        if (parentWidget() && pos().x() + newWidth > parentWidget()->width()) {
            newWidth = parentWidget()->width() - pos().x();
        }
        resize(newWidth, size().height());
    }
}

#include "kitemlistroleeditor.moc"
