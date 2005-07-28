/*
  Copyright (c) 2003 Dirk Mueller <mueller@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <qregexp.h>
#include <qlayout.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kurlrequester.h>

#include <q3whatsthis.h>
#include <kdebug.h>

#include <kapplication.h>
#include <dcopclient.h>
#include <kprocio.h>

extern "C"
{
    KDE_EXPORT void init_nsplugin()
    {
        KConfig *config = new KConfig("kcmnspluginrc", true /* readonly */, false /* no globals*/);
        config->setGroup("Misc");
        bool scan = config->readBoolEntry( "startkdeScan", false );
        bool firstTime = config->readBoolEntry( "firstTime", true );
        delete config;

        if ( scan || firstTime )
        {
            KApplication::kdeinitExec("nspluginscan");
        }

        if (firstTime) {
            config= new KConfig("kcmnspluginrc", false);
            config->setGroup("Misc");
            config->writeEntry( "firstTime", false );
            config->sync();
            delete config;
        }
    }
}
