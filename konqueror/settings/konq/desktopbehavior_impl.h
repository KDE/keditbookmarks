/**
 * (c) Martin R. Jones 1996
 * (c) David Faure 1998, 2000
 * (c) John Firebaugh 2003
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

#ifndef desktopbehavior_h
#define desktopbehavior_h

#include "desktopbehavior.h"
#include <kconfig.h>
#include <kcmodule.h>

class DesktopBehavior : public DesktopBehaviorBase
{
        Q_OBJECT
public:
        DesktopBehavior(KConfig *config, QWidget *parent = 0L, const char *name = 0L );
        virtual void load();
        virtual void save();
        virtual void defaults();
        virtual QString quickHelp() const;
        friend class DesktopBehaviorPreviewItem;
	friend class DesktopBehaviorDevicesItem;

signals:
        void changed();

private slots:
        void enableChanged();
	void comboBoxChanged();
	void editButtonPressed();

private:
        KConfig *g_pConfig;

	void fillDevicesListView();
	void saveDevicesListView();

        // Combo for the menus
        void fillMenuCombo( QComboBox * combo );

        typedef enum { NOTHING = 0, WINDOWLISTMENU, DESKTOPMENU, APPMENU } menuChoice;
        bool m_wheelSwitchesWorkspace;
};

class DesktopBehaviorModule : public KCModule
{
        Q_OBJECT

public:
        DesktopBehaviorModule(KConfig *config, QWidget *parent = 0L, const char *name = 0L );
        virtual void load() { m_behavior->load(); setChanged( false ); }
        virtual void save() { m_behavior->save(); setChanged( false ); }
        virtual void defaults() { m_behavior->defaults(); setChanged( true ); }

private slots:
        void changed();

private:
        DesktopBehavior* m_behavior;
};

#endif
