/**
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __desktop_h__
#define __desktop_h__

#include <kcmodule.h>

class QSpinBox;
class QLabel;
class QCheckBox;
class KLineEdit;
class KIntNumInput;

static const int maxDesktops = 20;

class KDesktopConfig : public KCModule
{
  Q_OBJECT
  
 public:
  KDesktopConfig(QWidget *parent = 0L, const char *name = 0L);

  void load();
  void save();
  void defaults();
  
  int buttons();
  QString quickHelp() const;

 protected slots:
  void slotValueChanged(int);
  void slotOptionChanged();
    
 private:
  KIntNumInput *_numInput;
  QLabel       *_nameLabel[maxDesktops];
  KLineEdit    *_nameInput[maxDesktops];
  QCheckBox    *_wheelOption;
};

#endif
