/*
   kmanualproxydlg.h - Base dialog box for proxy configuration

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

#ifndef KMANUAL_PROXY_DIALOG_H
#define KMANUAL_PROXY_DIALOG_H

#include "kproxydlgbase.h"

class QSpinBox;
class QGroupBox;
class QCheckBox;
class QPushButton;

class KLineEdit;
class KExceptionBox;

class KManualProxyDlg : public KProxyDialogBase
{
  Q_OBJECT

public:
  KManualProxyDlg( QWidget* parent = 0, const char* name = 0 );
  ~KManualProxyDlg();

  virtual void setProxyData( const KProxyData &data );
  virtual const KProxyData data() const;

protected:
  void init();
  bool validate();

protected slots:
  virtual void slotOk();

  void copyDown();
  void sameProxy( bool );
  void textChanged (const QString&);
  void valueChanged (int value);
  
private:
  QSpinBox* m_sbFtp;
  QSpinBox* m_sbHttp;
  QSpinBox* m_sbHttps;
  
  QLabel * m_lbFtp;
  QLabel * m_lbHttp;
  QLabel * m_lbHttps;  
  
  QCheckBox* m_cbSameProxy;

  KLineEdit* m_leFtp;
  KLineEdit* m_leHttp;
  KLineEdit* m_leHttps;

  QGroupBox* m_gbHostnames;
  KExceptionBox* m_gbExceptions;

  QPushButton* m_pbCopyDown;
  
  int m_oldFtpPort;
  int m_oldHttpsPort;  
  QString m_oldFtpText;
  QString m_oldHttpsText;
};
#endif
