/***************************************************************************
 *   Copyright (C) 2006-2011 by Peter Penz <peter.penz19@gmail.com>        *
 *   Copyright (C) 2006 by Holger 'zecke' Freyther <freyther@kde.org>      *
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

#ifndef DOLPHIN_APPLICATION_H
#define DOLPHIN_APPLICATION_H

#include <KApplication>

class DolphinMainWindow;

class DolphinApplication : public KApplication
{
    Q_OBJECT

public:
    DolphinApplication();
    virtual ~DolphinApplication();

    static DolphinApplication* app();

    void restoreSession();

private:
    DolphinMainWindow* m_mainWindow;
};

#endif
