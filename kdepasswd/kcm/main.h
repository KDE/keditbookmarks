
/**
 *  Copyright (C) 2004 Frans Englich <frans.englich@telia.com>
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
 */

#ifndef MAIN_H
#define MAIN_H

#include <kcmodule.h>

#include "main_widget.h"
//Added by qt3to4:
#include <QPixmap>
#include <QEvent>
#include <QDropEvent>

class KAboutData;
class KUser;
class KEmailSettings;
class QEvent;
class QObject;
class KURL;
class Config;

/**
 * Please see the README
 */
class KCMUserAccount : public KCModule
{
	Q_OBJECT

public:
	KCMUserAccount(QWidget* parent, const char* name = "KCMUserAccount",
		const QStringList& list=QStringList());
	~KCMUserAccount();

	/**
	 * The user data is loaded from  chfn(/etc/password) and then 
	 * written back as well as to KDE's own(KEmailSettings).
	 * The user won't notice this(assuming they change the KDE settings via 
	 * this KCM) and will make KDE play nice with enviroments which uses 
	 * /etc/password.
	 */
	void load();

	void save();

	/**
	 * For the face button
	 */
	bool eventFilter(QObject *, QEvent *e);

private slots:
	void slotChangePassword();
	//void configChanged() { emit changed(true); };
	void slotFaceButtonClicked();

private:
	void changeFace(const QPixmap& pix);
	inline KURL* decodeImgDrop(QDropEvent *e, QWidget *wdg);

	KEMailSettings *_kes;
	KUser *_ku;
	MainWidget *_mw;
	FacePerm _facePerm;
	QPixmap _facePixmap;

};

#endif // MAIN_H
