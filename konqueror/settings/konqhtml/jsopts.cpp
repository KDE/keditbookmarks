// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
// KControl port & modifications
// (c) Torben Weis 1998
// End of the KControl port, added 'kfmclient configure' call.
// (c) David Faure 1998
// New configuration scheme for JavaScript
// (C) Kalle Dalheimer 2000
// Major cleanup & Java/JS settings splitted
// (c) Daniel Molkentin 2000
// Big changes to accomodate per-domain settings
// (c) Leo Savernik 2002

#include <qlayout.h>
#include <qwhatsthis.h>
#include <qhgroupbox.h>
#include <kconfig.h>
#include <klistview.h>
#include <kdebug.h>
#include <kurlrequester.h>
#include <X11/Xlib.h>

#include "htmlopts.h"
#include "policydlg.h"

#include <klocale.h>

#include "jsopts.h"

#include "jsopts.moc"

// == class KJavaScriptOptions =====

KJavaScriptOptions::KJavaScriptOptions( KConfig* config, QString group, QWidget *parent,
										const char *name ) :
  KCModule( parent, name ),
  _removeJavaScriptDomainAdvice(false),
   m_pConfig( config ), m_groupname( group ),
  js_global_policies(config,group,true,QString::null),
  _removeECMADomainSettings(false)
{
  QVBoxLayout* toplevel = new QVBoxLayout( this, 10, 5 );

  // the global checkbox
  QHGroupBox* globalGB = new QHGroupBox( i18n( "Global Settings" ), this );
  toplevel->addWidget( globalGB );

  enableJavaScriptGloballyCB = new QCheckBox( i18n( "Ena&ble JavaScript globally" ), globalGB );
  QWhatsThis::add( enableJavaScriptGloballyCB, i18n("Enables the execution of scripts written in ECMA-Script "
        "(also known as JavaScript) that can be contained in HTML pages. "
        "Note that, as with any browser, enabling scripting languages can be a security problem.") );
  connect( enableJavaScriptGloballyCB, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );
  connect( enableJavaScriptGloballyCB, SIGNAL( clicked() ), this, SLOT( slotChangeJSEnabled() ) );

  reportErrorsCB = new QCheckBox( i18n( "Report errors" ), globalGB );
  QWhatsThis::add( reportErrorsCB, i18n("Enables the reporting of errors that occur when JavaScript "
	"code is executed.") );
  connect( reportErrorsCB, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );

  // the domain-specific listview
  domainSpecific = new JSDomainListView(m_pConfig,m_groupname,this);
  connect(domainSpecific,SIGNAL(changed(bool)),SLOT(slotChanged()));
  toplevel->addWidget( domainSpecific, 2 );

  QWhatsThis::add( domainSpecific, i18n("Here you can set specific JavaScript policies for any particular "
                                          "host or domain. To add a new policy, simply click the <i>New...</i> "
                                          "button and supply the necessary information requested by the "
                                          "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                          "button and choose the new policy from the policy dialog box. Clicking "
                                          "on the <i>Delete</i> button will remove the selected policy causing the default "
                                          "policy setting to be used for that domain. The <i>Import</i> and <i>Export</i> "
                                          "button allows you to easily share your policies with other people by allowing "
                                          "you to save and retrieve them from a zipped file.") );

  QString wtstr = i18n("This box contains the domains and hosts you have set "
                       "a specific JavaScript policy for. This policy will be used "
                       "instead of the default policy for enabling or disabling JavaScript on pages sent by these "
                       "domains or hosts. <p>Select a policy and use the controls on "
                       "the right to modify it.");
  QWhatsThis::add( domainSpecific->listView(), wtstr );

  QWhatsThis::add( domainSpecific->importButton(), i18n("Click this button to choose the file that contains "
                                        "the JavaScript policies. These policies will be merged "
                                        "with the existing ones. Duplicate entries are ignored.") );
  QWhatsThis::add( domainSpecific->exportButton(), i18n("Click this button to save the JavaScript policy to a zipped "
                                        "file. The file, named <b>javascript_policy.tgz</b>, will be "
                                        "saved to a location of your choice." ) );

  // the frame containing the JavaScript policies settings
  js_policies_frame = new JSPoliciesFrame(&js_global_policies,
  		i18n("Global JavaScript Policies"),this);
  toplevel->addWidget(js_policies_frame);
  connect(js_policies_frame, SIGNAL(changed()), SLOT(slotChanged()));

  // Finally do the loading
  load();
}


void KJavaScriptOptions::load()
{
    // *** load ***
    m_pConfig->setGroup(m_groupname);

    if( m_pConfig->hasKey( "ECMADomains" ) )
	domainSpecific->initialize(m_pConfig->readListEntry("ECMADomains"));
    else if( m_pConfig->hasKey( "ECMADomainSettings" ) ) {
        domainSpecific->updateDomainListLegacy( m_pConfig->readListEntry( "ECMADomainSettings" ) );
	_removeECMADomainSettings = true;
    } else {
        domainSpecific->updateDomainListLegacy(m_pConfig->readListEntry("JavaScriptDomainAdvice") );
	_removeJavaScriptDomainAdvice = true;
    }

    // *** apply to GUI ***
    js_policies_frame->load();
    enableJavaScriptGloballyCB->setChecked(
    		js_global_policies.isFeatureEnabled());
    reportErrorsCB->setChecked( m_pConfig->readBoolEntry("ReportJavaScriptErrors",true));
//    js_popup->setButton( m_pConfig->readUnsignedNumEntry("WindowOpenPolicy", 0) );
}

void KJavaScriptOptions::defaults()
{
  js_policies_frame->defaults();
  enableJavaScriptGloballyCB->setChecked(
    		js_global_policies.isFeatureEnabled());
  reportErrorsCB->setChecked( true );
}

void KJavaScriptOptions::save()
{
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "ReportJavaScriptErrors", reportErrorsCB->isChecked() );

    domainSpecific->save(m_groupname,"ECMADomains");
    js_policies_frame->save();

    if (_removeECMADomainSettings) {
      m_pConfig->deleteEntry("ECMADomainSettings");
      _removeECMADomainSettings = false;
    }

    // sync moved to KJSParts::save
//    m_pConfig->sync();
}

void KJavaScriptOptions::slotChanged()
{
  emit changed(true);
}

void KJavaScriptOptions::slotChangeJSEnabled() {
  js_global_policies.setFeatureEnabled(enableJavaScriptGloballyCB->isChecked());
}

// == class JSDomainListView =====

JSDomainListView::JSDomainListView(KConfig *config,const QString &group,
	QWidget *parent,const char *name)
	: DomainListView(config,i18n( "Do&main-Specific" ), parent, name),
	group(group) {
}

JSDomainListView::~JSDomainListView() {
}

void JSDomainListView::updateDomainListLegacy(const QStringList &domainConfig)
{
    domainSpecificLV->clear();
    JSPolicies pol(config,group,false);
    pol.defaults();
    for (QStringList::ConstIterator it = domainConfig.begin();
         it != domainConfig.end(); ++it) {
      QString domain;
      KHTMLSettings::KJavaScriptAdvice javaAdvice;
      KHTMLSettings::KJavaScriptAdvice javaScriptAdvice;
      KHTMLSettings::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
      if (javaScriptAdvice != KHTMLSettings::KJavaScriptDunno) {
        QListViewItem *index =
          new QListViewItem( domainSpecificLV, domain,
                i18n(KHTMLSettings::adviceToStr(javaScriptAdvice)) );

        pol.setDomain(domain);
        pol.setFeatureEnabled(javaScriptAdvice != KHTMLSettings::KJavaScriptReject);
        domainPolicies[index] = new JSPolicies(pol);
      }
    }
}

void JSDomainListView::setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
		Policies *pol) {
  JSPolicies *jspol = static_cast<JSPolicies *>(pol);
  QString caption;
  switch (trigger) {
    case AddButton: caption = i18n( "New JavaScript Policy" ); break;
    case ChangeButton: caption = i18n( "Change JavaScript Policy" ); break;
    default: ; // inhibit gcc warning
  }/*end switch*/
  pDlg.setCaption(caption);
  pDlg.setFeatureEnabledLabel(i18n("JavaScript policy:"));
  pDlg.setFeatureEnabledWhatsThis(i18n("Select a JavaScript policy for "
                                          "the above host or domain."));
  JSPoliciesFrame *panel = new JSPoliciesFrame(jspol,i18n("Domain-specific "
  				"JavaScript policies"),&pDlg);
  panel->refresh();
  pDlg.addPolicyPanel(panel);
  pDlg.refresh();
}

JSPolicies *JSDomainListView::createPolicies() {
  return new JSPolicies(config,group,false);
}

JSPolicies *JSDomainListView::copyPolicies(Policies *pol) {
  return new JSPolicies(*static_cast<JSPolicies *>(pol));
}


