/* This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
*/

#ifndef DOLPHINCATEGORYDRAWER_H
#define DOLPHINCATEGORYDRAWER_H

#include <kcategorydrawer.h>

#include <QStyleOption>
#include <QModelIndex>

#include <libdolphin_export.h>

class LIBDOLPHINPRIVATE_EXPORT DolphinCategoryDrawer
    : public KCategoryDrawer
{
public:
    DolphinCategoryDrawer();

    virtual ~DolphinCategoryDrawer();

    virtual void drawCategory(const QModelIndex &index, int sortRole,
                              const QStyleOption &option, QPainter *painter) const;

    virtual int categoryHeight(const QModelIndex &index, const QStyleOption &option) const;
};

#endif // DOLPHINCATEGORYDRAWER_H
