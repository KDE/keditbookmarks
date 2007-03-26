//-----------------------------------------------------------------------------
//
// Plugin Options
//
// (c) 2002 Leo Savernik, per-domain settings
// (c) 2001, Daniel Naber, based on javaopts.h
// (c) 2000, Stefan Schimanski <1Stein@gmx.de>, Netscape parts
//
//-----------------------------------------------------------------------------

#ifndef __PLUGINOPTS_H__
#define __PLUGINOPTS_H__

#include <QWidget>

#include "domainlistview.h"
#include "policies.h"

class QCheckBox;

#include <kcmodule.h>
#include <kconfig.h>
#include "ui_nsconfigwidget.h"

class QBoxLayout;
class QLabel;
class QProgressDialog;
class QSlider;
class KDialog;
class KPluginOptions;
class K3ProcIO;
namespace Ui {
class NSConfigWidget;
}

/** policies with plugin-specific constructor
  */
class PluginPolicies : public Policies {
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
  PluginPolicies(KSharedConfig::Ptr config, const QString &group, bool global,
  		const QString &domain = QString());

  virtual ~PluginPolicies();
};

/** Plugin-specific enhancements to the domain list view
  */
class PluginDomainListView : public DomainListView {
  Q_OBJECT
public:
  PluginDomainListView(KSharedConfig::Ptr config,const QString &group,KPluginOptions *opt,
                       QWidget *parent);
  virtual ~PluginDomainListView();

protected:
  virtual PluginPolicies *createPolicies();
  virtual PluginPolicies *copyPolicies(Policies *pol);
  virtual void setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
  		Policies *copy);

private:
  QString group;
  KPluginOptions *options;
};

/**
 * dialog for embedding a PluginDomainListView widget
 */
class PluginDomainDialog : public QWidget {
  Q_OBJECT
public:

  PluginDomainDialog(QWidget *parent);
  virtual ~PluginDomainDialog();

  void setMainWidget(QWidget *widget);

private Q_SLOTS:
  virtual void slotClose();

private:
  PluginDomainListView *domainSpecific;
  QBoxLayout *thisLayout;
};

class KPluginOptions : public KCModule
{
    Q_OBJECT

public:
    KPluginOptions( QWidget* parent, const QStringList& );

    virtual void load();
    virtual void save();
    virtual void defaults();
    QString quickHelp() const;

private Q_SLOTS:
    void slotChanged();
    void slotTogglePluginsEnabled();
    void slotShowDomainDlg();

private:

    KSharedConfig::Ptr m_pConfig;
    QString  m_groupname;

    QCheckBox *enablePluginsGloballyCB, *enableHTTPOnly, *enableUserDemand;


 protected Q_SLOTS:
  void progress(K3ProcIO *);
  void updatePLabel(int);
  void change() { change( true ); }
  void change( bool c ) { emit changed(c); m_changed = c; }

  void scan();
  void scanDone();

 private:
  Ui::NSConfigWidget *m_widget;
  bool m_changed;
  QProgressDialog *m_progress;
  QSlider *priority;
  QLabel *priorityLabel;
  PluginPolicies global_policies;
  PluginDomainListView *domainSpecific;
  KDialog *domainSpecificDlg;

/******************************************************************************/
 protected:
  void dirInit();
  void dirLoad( KSharedConfig::Ptr config, bool useDefault= false );
  void dirSave( KSharedConfig::Ptr config );

 protected Q_SLOTS:
  void dirNew();
  void dirRemove();
  void dirUp();
  void dirDown();
  void dirEdited(const QString &);
  void dirSelect( QListWidgetItem * );

/******************************************************************************/
 protected:
  void pluginInit();
  void pluginLoad( KSharedConfig::Ptr config );
  void pluginSave( KSharedConfig::Ptr config );

  friend class PluginDomainListView;
};

#endif		// __PLUGINOPTS_H__
