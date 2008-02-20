/*
 * Copyright (C) Jacek Konieczy <jajcus@zeus.polsl.gliwice.pl>
 *     extra HTTP configuration
 *
 */

#ifndef __KHTTPOPTDLG_H
#define __KHTTPOPTDLG_H

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>


#include <kcmodule.h>
#include <kconfig.h>
#include <ksharedconfig.h>

/**
*  Dialog for configuring HTTP Options like charset and language negotiation
*  and assuming that file got from HTTP is HTML if no Content-Type is given
*/
class KHTTPOptions : public KCModule
{
Q_OBJECT
  public:
    KHTTPOptions(KSharedConfig::Ptr config, const QString &group, const KComponentData &componentData, QWidget *parent);

    virtual void load();
    virtual void save();
    virtual void defaults();

  private:

    KSharedConfig::Ptr m_pConfig;
    QString m_groupname;

    // Acceptable languages "LANG" - locale selected languages
    QLabel *lb_languages;	
    QLineEdit *le_languages;	

    // Acceptable charsets "CHARSET" - locale selected charset
    QLabel *lb_charsets;	
    QLineEdit *le_charsets;	

    QString defaultCharsets;

private Q_SLOTS:
    void slotChanged();

};

#endif // __KHTTPOPTDLG_H
