/*
   kmanualproxydlg.h - Base dialog box for proxy configuration

   Copyright (C) 2001-2004 Dawit Alemayehu <adawit@kde.org>

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

#ifndef KMANUALPROXYDLG_H
#define KMANUALPROXYDLG_H

#include "kproxydlgbase.h"
#include "ui_manualproxy.h"
class QSpinBox;
class KLineEdit;

class ManualProxyDlgUI : public QWidget, public Ui::ManualProxyDlgUI
{
public:
  ManualProxyDlgUI( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class KManualProxyDlg : public KProxyDialogBase
{
  Q_OBJECT
  
public:
  explicit KManualProxyDlg( QWidget* parent = 0, const char* name = 0 );
  ~KManualProxyDlg() {}
  
  virtual void setProxyData( const KProxyData &data );
  virtual const KProxyData data() const;
  
protected:
  void init();
  bool validate();
  
protected Q_SLOTS:
  virtual void slotOk();
  
  void copyDown();
  void sameProxy( bool );
  void valueChanged (int value);
  void textChanged (const QString&);
  
  void newPressed();
  void updateButtons();
  void changePressed();
  void deletePressed();
  void deleteAllPressed();
  
private:
  QString urlFromInput( const KLineEdit* edit, const QSpinBox* spin ) const;
  bool isValidURL( const QString&, KUrl* = 0 ) const;
  bool handleDuplicate( const QString& );
  bool getException ( QString&, const QString&,
                      const QString& value = QString() );
  void showErrorMsg( const QString& caption = QString(),
                     const QString& message = QString() );
  
private:
  ManualProxyDlgUI* mDlg;

  int mOldFtpPort;
  int mOldHttpsPort;
  QString mOldFtpText;
  QString mOldHttpsText;
};

#endif // KMANUALPROXYDLG_H
