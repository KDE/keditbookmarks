/* -*- indent-tabs-mode: t; tab-width: 4; c-basic-offset:4 -*-

    konq_extensionmanager.h - Extension Manager for Konqueror

    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>
    Copyright (c) 2004      by Arend van Beelen jr.  <arend@auton.nl>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KONQEXTENSIONMANAGER_H
#define KONQEXTENSIONMANAGER_H

#include <kdialogbase.h>

class KonqExtensionManagerPrivate;
class KonqMainWindow;
namespace KParts { class ReadOnlyPart; }

/**
 * Extension Manager for Konqueror. See KPluginSelector in kdelibs for
 * documentation.
 *
 * @author Martijn Klingens <klingens@kde.org>
 * @author Arend van Beelen jr. <arend@auton.nl>
 */
class KonqExtensionManager : public KDialogBase
{
	Q_OBJECT

	public:
		KonqExtensionManager(QWidget *parent, KonqMainWindow *mainWindow, KParts::ReadOnlyPart* activePart);
		~KonqExtensionManager();

		void apply();

	public slots:
		void setChanged(bool c);

		virtual void slotDefault();
		virtual void slotUser1();
		virtual void slotApply();
		virtual void slotOk();
		virtual void slotHelp();
		virtual void show();

	private:
		KonqExtensionManagerPrivate *d;
};

#endif // KONQEXTENSIONMANAGER_H
