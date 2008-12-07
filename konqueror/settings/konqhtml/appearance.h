// -*- c-basic-offset: 2 -*-
/*
 * Copyright (c) Martin R. Jones 1996
 * Copyright (c) Bernd Wuebben 1998
 *
 * Copyright (c) Torben Weis 1998
 *     KControl port & modifications
 *
 * Copyright (c) David Faure 1998
 *     End of the KControl port, added 'kfmclient configure' call.
 *
 * Copyright (c) Daniel Molkentin 2000
 *     Cleanup and modifications for KDE 2.1
 *
 */

#ifndef APPEARANCE_H
#define APPEARANCE_H

#include <QtGui/QWidget>
#include <QtCore/QMap>

#include <kcmodule.h>
#include <kconfig.h>

class QSpinBox;
class KFontComboBox;
class QComboBox;
class QCheckBox;
class CSSConfig;


class KAppearanceOptions : public KCModule
{
  Q_OBJECT
public:
  KAppearanceOptions(QWidget *parent, const QVariantList&);
  ~KAppearanceOptions();

  virtual void load();
  virtual void save();
  virtual void defaults();

public Q_SLOTS:
  void slotFontSize( int );
  void slotMinimumFontSize( int );
  void slotStandardFont(const QFont& n);
  void slotFixedFont(const QFont& n);
  void slotSerifFont( const QFont& n );
  void slotSansSerifFont( const QFont& n );
  void slotCursiveFont( const QFont& n );
  void slotFantasyFont( const QFont& n );
  void slotEncoding( const QString& n);
  void slotFontSizeAdjust( int value );

private:
  void updateGUI();

private:
  CSSConfig* cssConfig;
  
  QCheckBox* m_pAutoLoadImagesCheckBox;
  QCheckBox* m_pUnfinishedImageFrameCheckBox;
  QComboBox* m_pAnimationsCombo;
  QComboBox* m_pUnderlineCombo;
  QComboBox* m_pSmoothScrollingCombo;



  KSharedConfig::Ptr m_pConfig;
  QString m_groupname;

  KIntNumInput* m_minSize;
  KIntNumInput* m_MedSize;
  KIntNumInput* m_pageDPI;
  KFontComboBox* m_pFonts[6];
  QComboBox* m_pEncoding;
  QSpinBox *m_pFontSizeAdjust;

  int fSize;
  int fMinSize;
  QStringList encodings;
  QStringList fonts;
  QStringList defaultFonts;
  QString encodingName;
};

#endif // APPEARANCE_H
