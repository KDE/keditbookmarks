/*
 *  Copyright (c) 2000 Yves Arrouye <yves@realnames.com>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef MAIN_H
#define MAIN_H

#include <kcmodule.h>

class KUriFilter;

class KURIFilterModule : public KCModule {
    Q_OBJECT

public:
    KURIFilterModule(QWidget *parent, const QVariantList &);
    ~KURIFilterModule();

    void load();
    void save();
    void defaults();

private:
    KUriFilter *filter;

    QWidget *m_widget;
    QList<KCModule *> modules;
};

#endif
