/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
 *   Copyright (C) 2006 by Aaron J. Seigo (<aseigo@kde.org>)               *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "urlnavigatorbutton.h"
#include <kurl.h>
#include <qtooltip.h>
#include <qcursor.h>
#include <qfontmetrics.h>
//Added by qt3to4:
#include <QEvent>
#include <kiconloader.h>
#include <klocale.h>

#include "urlnavigator.h"
#include "dolphin.h"


URLButton::URLButton(URLNavigator* parent)
: QPushButton(parent),
  m_displayHint(0),
  m_urlNavigator(parent)
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    setMinimumHeight(parent->minimumHeight());

    connect(this, SIGNAL(clicked()), parent, SLOT(slotRequestActivation()));
    connect(&Dolphin::mainWin(), SIGNAL(activeViewChanged()),
            this, SLOT(update()));
}

URLButton::~URLButton()
{
}

URLNavigator* URLButton::urlNavigator() const
{
    return m_urlNavigator;
}

void URLButton::setDisplayHintEnabled(DisplayHint hint,
                                      bool enable)
{
    if (enable) {
        m_displayHint = m_displayHint | hint;
    }
    else {
        m_displayHint = m_displayHint & ~hint;
    }
    update();
}

bool URLButton::isDisplayHintEnabled(DisplayHint hint) const
{
    return (m_displayHint & hint) > 0;
}

void URLButton::enterEvent(QEvent* event)
{
    QPushButton::enterEvent(event);
    setDisplayHintEnabled(EnteredHint, true);
    update();
}

void URLButton::leaveEvent(QEvent* event)
{
    QPushButton::leaveEvent(event);
    setDisplayHintEnabled(EnteredHint, false);
    update();
}

QColor URLButton::mixColors(const QColor& c1,
                            const QColor& c2) const
{
    const int red   = (c1.red()   + c2.red())   / 2;
    const int green = (c1.green() + c2.green()) / 2;
    const int blue  = (c1.blue()  + c2.blue())  / 2;
    return QColor(red, green, blue);
}
