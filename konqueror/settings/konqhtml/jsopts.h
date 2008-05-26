//-----------------------------------------------------------------------------
//
// HTML Options
//
// (c) Martin R. Jones 1996
//
// Port to KControl
// (c) Torben Weis 1998

#ifndef JSOPTS_H
#define JSOPTS_H

#include <kcmodule.h>

#include "domainlistview.h"
#include "jspolicies.h"

class QCheckBox;

class PolicyDialog;

class KJavaScriptOptions;

/** JavaScript-specific enhancements to the domain list view
  */
class JSDomainListView : public DomainListView {
  Q_OBJECT
public:
  JSDomainListView(KSharedConfig::Ptr config,const QString &group,KJavaScriptOptions *opt,
  		QWidget *parent);
  virtual ~JSDomainListView();

  /** remnant for importing pre KDE 3.2 settings
    */
  void updateDomainListLegacy(const QStringList &domainConfig);

protected:
  virtual JSPolicies *createPolicies();
  virtual JSPolicies *copyPolicies(Policies *pol);
  virtual void setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
  		Policies *copy);

private:
  QString group;
  KJavaScriptOptions *options;
};

class KJavaScriptOptions : public KCModule
{
  Q_OBJECT
public:
  KJavaScriptOptions( KSharedConfig::Ptr config, const QString &group, const KComponentData &componentData, QWidget* parent );

  virtual void load();
  virtual void save();
  virtual void defaults();

  bool _removeJavaScriptDomainAdvice;

private Q_SLOTS:
  void slotChangeJSEnabled();

private:

  KSharedConfig::Ptr m_pConfig;
  QString m_groupname;
  JSPolicies js_global_policies;
  QCheckBox *enableJavaScriptGloballyCB;
  QCheckBox *reportErrorsCB;
  QCheckBox *jsDebugWindow;
  JSPoliciesFrame *js_policies_frame;
  bool _removeECMADomainSettings;

  JSDomainListView* domainSpecific;
  
  friend class JSDomainListView;
};

#endif		// JSOPTS_H

