/*
   cache.h - Proxy configuration dialog

   Copyright (C) 2001- Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License (GPL) version 2 as published by the Free Software
   Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef CACHE_H
#define CACHE_H

#include <kcmodule.h>


class QLabel;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QRadioButton;
class QButtonGroup;

class KCacheConfigDialog : public KCModule
{
    Q_OBJECT

public:
    KCacheConfigDialog( QWidget* parent = 0, const char* name = 0 );
    ~KCacheConfigDialog();

    virtual void load();
    virtual void save();
    virtual void defaults();
    QString quickHelp() const;

protected slots:
  void configChanged() { emit changed( true ); };
  void slotClearCache();

private:
    QCheckBox* cb_useCache;

    QButtonGroup* gb_Cache_policy;
    QRadioButton* rb_verify;
    QRadioButton* rb_cacheIfPossible;
    QRadioButton* rb_offlineMode;

    QLabel* lb_max_cache_size;
    QSpinBox* sb_max_cache_size;
    QPushButton* pb_clearCache;
};

#endif // CACHE_H
