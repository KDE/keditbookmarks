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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "kstandarditem.h"

#include <KDebug>

KStandardItem::KStandardItem(KStandardItem* parent) :
    m_parent(parent),
    m_children(),
    m_model(0),
    m_data()
{
}

KStandardItem::KStandardItem(const QString& text, KStandardItem* parent) :
    m_parent(parent),
    m_children(),
    m_model(0),
    m_data()
{
    setText(text);
}

KStandardItem::KStandardItem(const QIcon& icon, const QString& text, KStandardItem* parent) :
    m_parent(parent),
    m_children(),
    m_model(0),
    m_data()
{
    setIcon(icon);
    setText(text);
}

KStandardItem::KStandardItem(const KStandardItem& item) :
    m_parent(item.m_parent),
    m_children(item.m_children),
    m_model(item.m_model),
    m_data(item.m_data)
{
}

KStandardItem::~KStandardItem()
{
}

void KStandardItem::setText(const QString& text)
{
    m_data.insert("text", text);
}

QString KStandardItem::text() const
{
    return m_data["text"].toString();
}

void KStandardItem::setIcon(const QIcon& icon)
{
    m_data.insert("iconName", icon.name());
}

QIcon KStandardItem::icon() const
{
    return QIcon(m_data["iconName"].toString());
}

void KStandardItem::setGroup(const QString& group)
{
    m_data.insert("group", group);
}

QString KStandardItem::group() const
{
    return m_data["group"].toString();
}

void KStandardItem::setDataValue(const QByteArray& role, const QVariant& value)
{
    m_data.insert(role, value);
}

QVariant KStandardItem::dataValue(const QByteArray& role) const
{
    return m_data[role];
}

void KStandardItem::setParent(KStandardItem* parent)
{
    // TODO: not implemented yet
    m_parent = parent;
}

KStandardItem* KStandardItem::parent() const
{
    return m_parent;
}

void KStandardItem::setData(const QHash<QByteArray, QVariant>& values)
{
    m_data = values;
}

QHash<QByteArray, QVariant> KStandardItem::data() const
{
    return m_data;
}

QList<KStandardItem*> KStandardItem::children() const
{
    return m_children;
}
