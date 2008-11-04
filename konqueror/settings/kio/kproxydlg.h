/*
   kproxydlg.h - Proxy configuration dialog

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KPROXYDLG_H
#define KPROXYDLG_H

#include <kcmodule.h>
#include "kproxydlgbase.h"
#include "ui_kproxydlg.h"

class KProxyDialog : public KCModule
{
  Q_OBJECT

public:
  KProxyDialog(QWidget *parent, const QVariantList &args);
  ~KProxyDialog();

  virtual void load();
  virtual void save();
  virtual void defaults();
  QString quickHelp() const;

private Q_SLOTS:
  void slotChanged();
  void slotUseProxyChanged();

  void setupManProxy();
  void setupEnvProxy();

private:
  void showInvalidMessage( const QString& _msg = QString() );

private:
  Ui::KProxyDialogUI mUi;
  KProxyData mData;
  bool mDefaultData;
};

#endif // KPROXYDLG_H
