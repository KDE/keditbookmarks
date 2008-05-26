/*
 * Copyright (c) Martin R. Jones 1996
 *     HTML Options
 * Copyright (c) Torben Weis 1998 
 *     Port to KControl
 * Copyright (c) Daniel Molkentin 2000
 *     Redesign and cleanup
 *
 */

#ifndef JAVAOPTS_H
#define JAVAOPTS_H

#include <kcmodule.h>

#include "domainlistview.h"
#include "policies.h"

class KUrlRequester;
class KIntNumInput;

class QCheckBox;
class QLineEdit;

class KJavaOptions;

/** policies with java-specific constructor
  */
class JavaPolicies : public Policies {
public:
  /**
   * constructor
   * @param config configuration to initialize this instance from
   * @param group config group to use if this instance contains the global
   *	policies (global == true)
   * @param global true if this instance contains the global policy settings,
   *	false if this instance contains policies specific for a domain.
   * @param domain name of the domain this instance is used to configure the
   *	policies for (case insensitive, ignored if global == true)
   */
  JavaPolicies(KSharedConfig::Ptr config, const QString &group, bool global,
  		const QString &domain = QString());

  /** empty constructur to make QMap happy
   * don't use for constructing a policies instance.
   * @internal
   */
  //JavaPolicies();

  virtual ~JavaPolicies();
};

/** Java-specific enhancements to the domain list view
  */
class JavaDomainListView : public DomainListView {
  Q_OBJECT
public:
  JavaDomainListView(KSharedConfig::Ptr config,const QString &group,KJavaOptions *opt,
                     QWidget *parent);
  virtual ~JavaDomainListView();

  /** remnant for importing pre KDE 3.2 settings
    */
  void updateDomainListLegacy(const QStringList &domainConfig);

protected:
  virtual JavaPolicies *createPolicies();
  virtual JavaPolicies *copyPolicies(Policies *pol);
  virtual void setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
  		Policies *copy);

private:
  QString group;
  KJavaOptions *options;
};

class KJavaOptions : public KCModule
{
    Q_OBJECT

public:
    KJavaOptions( KSharedConfig::Ptr config, const QString &group, const KComponentData &componentData, QWidget* parent );

    virtual void load();
    virtual void save();
    virtual void defaults();

    bool _removeJavaScriptDomainAdvice;

private Q_SLOTS:
    void slotChanged();
    void toggleJavaControls();

private:

    KSharedConfig::Ptr m_pConfig;
    QString  m_groupname;
    JavaPolicies java_global_policies;

    QCheckBox*     enableJavaGloballyCB;
    QCheckBox*     javaSecurityManagerCB;
    QCheckBox*     useKioCB;
    QCheckBox*     enableShutdownCB;
    KIntNumInput*  serverTimeoutSB;
    QLineEdit*     addArgED;
    KUrlRequester* pathED;
    bool           _removeJavaDomainSettings;

    JavaDomainListView *domainSpecific;

    friend class JavaDomainListView;
};

#endif		// HTML_OPTIONS_H

