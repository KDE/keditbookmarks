// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
// KControl port & modifications
// (c) Torben Weis 1998
// End of the KControl port, added 'kfmclient configure' call.
// (c) David Faure 1998
// New configuration scheme for Java/JavaScript
// (c) Kalle Dalheimer 2000
// Redesign and cleanup
// (c) Daniel Molkentin 2000
// Big changes to accommodate per-domain settings
// (c) Leo Savernik 2002

#include <config.h>
#include <klistview.h>
#include <kurlrequester.h>
#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>
#include <khtml_settings.h>
#include <knuminput.h>

#include <qlayout.h>
#include <qwhatsthis.h>
#include <qvgroupbox.h>
#include <qlabel.h>

#include "htmlopts.h"
#include "policydlg.h"
#include "javaopts.h"

// == class JavaPolicies =====

JavaPolicies::JavaPolicies(KConfig* config, const QString &group, bool global,
  		const QString &domain) :
	Policies(config,group,global,domain,"java.","EnableJava") {
}

JavaPolicies::JavaPolicies() : Policies(0,QString::null,false,
	QString::null,QString::null,QString::null) {
}

JavaPolicies::~JavaPolicies() {
}

// == class KJavaOptions =====

KJavaOptions::KJavaOptions( KConfig* config, QString group,
                            QWidget *parent, const char *name )
    : KCModule( parent, name ),
      _removeJavaScriptDomainAdvice(false),
      m_pConfig( config ),
      m_groupname( group ),
      java_global_policies(config,group,true),
      _removeJavaDomainSettings(false)
{
    QVBoxLayout* toplevel = new QVBoxLayout( this, 10, 5 );

    /***************************************************************************
     ********************* Global Settings *************************************
     **************************************************************************/
    QVGroupBox* globalGB = new QVGroupBox( i18n( "Global Settings" ), this );
    toplevel->addWidget( globalGB );
    enableJavaGloballyCB = new QCheckBox( i18n( "Enable Ja&va globally" ), globalGB );
    connect( enableJavaGloballyCB, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );
    connect( enableJavaGloballyCB, SIGNAL( clicked() ), this, SLOT( toggleJavaControls() ) );


    /***************************************************************************
     ***************** Domain Specific Settings ********************************
     **************************************************************************/
    domainSpecific = new JavaDomainListView(m_pConfig,m_groupname,this);
    connect(domainSpecific,SIGNAL(changed(bool)),SLOT(slotChanged()));
    toplevel->addWidget( domainSpecific, 2 );

    /***************************************************************************
     ***************** Java Runtime Settings ***********************************
     **************************************************************************/
    QVGroupBox* javartGB = new QVGroupBox( i18n( "Java Runtime Settings" ), this );
    toplevel->addWidget( javartGB );

    QWidget* checkboxes = new QWidget( javartGB );
    QGridLayout* grid = new QGridLayout( checkboxes, 2, 2 );
    javaConsoleCB = new QCheckBox( i18n( "Sho&w Java console" ), checkboxes );
    grid->addWidget( javaConsoleCB, 0, 0 );
    connect( javaConsoleCB, SIGNAL(toggled( bool )), this, SLOT(slotChanged()) );

    javaSecurityManagerCB = new QCheckBox( i18n("&Use security manager" ), checkboxes );
    grid->addWidget( javaSecurityManagerCB, 0, 1 );
    connect( javaSecurityManagerCB, SIGNAL(toggled( bool )), this, SLOT(slotChanged()) );

    useKioCB = new QCheckBox( i18n("Use &KIO"), checkboxes );
    grid->addWidget( useKioCB, 1, 0 );
    connect( useKioCB, SIGNAL(toggled( bool )), this, SLOT(slotChanged()) );

    enableShutdownCB = new QCheckBox( i18n("Shu&tdown applet server when inactive"), checkboxes );
    grid->addWidget( enableShutdownCB, 1, 1 );
    connect( enableShutdownCB, SIGNAL(toggled( bool )), this, SLOT(slotChanged()) );
    connect( enableShutdownCB, SIGNAL(clicked()), this, SLOT(toggleJavaControls()) );

    QHBox* secondsHB = new QHBox( javartGB );
    serverTimeoutSB = new KIntNumInput( secondsHB );
    serverTimeoutSB->setRange( 0, 1000, 5 );
    serverTimeoutSB->setLabel( i18n("App&let server timeout:"), AlignLeft );
    serverTimeoutSB->setSuffix(i18n(" sec"));
    connect(serverTimeoutSB, SIGNAL(valueChanged(int)),this,SLOT(slotChanged()));

    QHBox* pathHB = new QHBox( javartGB );
    pathHB->setSpacing( 10 );
    QLabel* pathLA = new QLabel( i18n( "&Path to Java executable, or 'java':" ),
                                 pathHB );
    pathED = new  KURLRequester( pathHB );
    connect( pathED, SIGNAL(textChanged( const QString& )), this, SLOT(slotChanged()) );
    pathLA->setBuddy( pathED );

    QHBox* addArgHB = new QHBox( javartGB );
    addArgHB->setSpacing( 10 );
    QLabel* addArgLA = new QLabel( i18n( "Additional Java a&rguments:" ), addArgHB  );
    addArgED = new QLineEdit( addArgHB );
    connect( addArgED, SIGNAL(textChanged( const QString& )), this, SLOT(slotChanged()) );
    addArgLA->setBuddy( addArgED );

    /***************************************************************************
     ********************** WhatsThis? items ***********************************
     **************************************************************************/
    QWhatsThis::add( enableJavaGloballyCB, i18n("Enables the execution of scripts written in Java "
          "that can be contained in HTML pages. "
          "Note that, as with any browser, enabling active contents can be a security problem.") );
    QString wtstr = i18n("This box contains the domains and hosts you have set "
                         "a specific Java policy for. This policy will be used "
                         "instead of the default policy for enabling or disabling Java applets on pages sent by these "
                         "domains or hosts. <p>Select a policy and use the controls on "
                         "the right to modify it.");
    QWhatsThis::add( domainSpecific->listView(), wtstr );
#if 0
    QWhatsThis::add( domainSpecific->importButton(), i18n("Click this button to choose the file that contains "
                                          "the Java policies. These policies will be merged "
                                          "with the existing ones. Duplicate entries are ignored.") );
    QWhatsThis::add( domainSpecific->exportButton(), i18n("Click this button to save the Java policy to a zipped "
                                          "file. The file, named <b>java_policy.tgz</b>, will be "
                                          "saved to a location of your choice." ) );
#endif
    QWhatsThis::add( domainSpecific, i18n("Here you can set specific Java policies for any particular "
                                            "host or domain. To add a new policy, simply click the <i>New...</i> "
                                            "button and supply the necessary information requested by the "
                                            "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                            "button and choose the new policy from the policy dialog box. Clicking "
                                            "on the <i>Delete</i> button will remove the selected policy, causing the default "
                                            "policy setting to be used for that domain.") );
#if 0
                                            "The <i>Import</i> and <i>Export</i> "
                                            "button allows you to easily share your policies with other people by allowing "
                                            "you to save and retrieve them from a zipped file.") );
#endif

    QWhatsThis::add( javaConsoleCB, i18n( "If this box is checked, Konqueror will open a console window that Java programs "
                                          "can use for character-based input/output. Well-written Java applets do not need "
                                          "this, but the console can help to find problems with Java applets.") );

    QWhatsThis::add( javaSecurityManagerCB, i18n( "Enabling the security manager will cause the jvm to run with a Security "
                                                  "Manager in place. This will keep applets from being able to read and "
                                                  "write to your file system, creating arbitrary sockets, and other actions "
                                                  "which could be used to compromise your system. Disable this option at your "
                                                  "own risk. You can modify your $HOME/.java.policy file with the Java "
                                                  "policytool utility to give code downloaded from certain sites more "
                                                  "permissions." ) );

    QWhatsThis::add( useKioCB, i18n( "Enabling this will cause the jvm to use KIO for network transport ") );

    QWhatsThis::add( pathED, i18n("Enter the path to the java executable. If you want to use the jre in "
                                  "your path, simply leave it as 'java'. If you need to use a different jre, "
                                  "enter the path to the java executable (e.g. /usr/lib/jdk/bin/java), "
                                  "or the path to the directory that contains 'bin/java' (e.g. /opt/IBMJava2-13).") );

    QWhatsThis::add( addArgED, i18n("If you want special arguments to be passed to the virtual machine, enter them here.") );

    QString shutdown = i18n("When all the applets have been destroyed, the applet server should shut down. "
                                           "However, starting the jvm takes a lot of time. If you would like to "
                                           "keep the java process running while you are "
                                           "browsing, you can set the timeout value to whatever you like. To keep "
                                           "the java process running for the whole time that the konqueror process is, "
                                           "leave the Shutdown Applet Server checkbox unchecked.");
    QWhatsThis::add( serverTimeoutSB, shutdown);
    QWhatsThis::add( enableShutdownCB, shutdown);
    // Finally do the loading
    load();
}

void KJavaOptions::load()
{
    // *** load ***
    java_global_policies.load();
    bool bJavaGlobal      = java_global_policies.isFeatureEnabled();
    bool bJavaConsole     = m_pConfig->readBoolEntry( "ShowJavaConsole", false );
    bool bSecurityManager = m_pConfig->readBoolEntry( "UseSecurityManager", true );
    bool bUseKio = m_pConfig->readBoolEntry( "UseKio", false );
    bool bServerShutdown  = m_pConfig->readBoolEntry( "ShutdownAppletServer", true );
    int  serverTimeout    = m_pConfig->readNumEntry( "AppletServerTimeout", 60 );
#if defined(PATH_JAVA)
    QString sJavaPath     = m_pConfig->readPathEntry( "JavaPath", PATH_JAVA );
#else
    QString sJavaPath     = m_pConfig->readPathEntry( "JavaPath", "java" );
#endif

    if( sJavaPath == "/usr/lib/jdk" )
        sJavaPath = "java";

    if( m_pConfig->hasKey( "JavaDomains" ) )
    	domainSpecific->initialize(m_pConfig->readListEntry("JavaDomains"));
    else if( m_pConfig->hasKey( "JavaDomainSettings" ) ) {
        domainSpecific->updateDomainListLegacy( m_pConfig->readListEntry("JavaDomainSettings") );
	_removeJavaDomainSettings = true;
    } else {
        domainSpecific->updateDomainListLegacy( m_pConfig->readListEntry("JavaScriptDomainAdvice") );
	_removeJavaScriptDomainAdvice = true;
    }

    // *** apply to GUI ***
    enableJavaGloballyCB->setChecked( bJavaGlobal );
    javaConsoleCB->setChecked( bJavaConsole );
    javaSecurityManagerCB->setChecked( bSecurityManager );
    useKioCB->setChecked( bUseKio );

    addArgED->setText( m_pConfig->readEntry( "JavaArgs" ) );
    pathED->lineEdit()->setText( sJavaPath );

    enableShutdownCB->setChecked( bServerShutdown );
    serverTimeoutSB->setValue( serverTimeout );

    toggleJavaControls();
}

void KJavaOptions::defaults()
{
    java_global_policies.defaults();
    enableJavaGloballyCB->setChecked( false );
    javaConsoleCB->setChecked( false );
    javaSecurityManagerCB->setChecked( true );
    useKioCB->setChecked( false );
    pathED->lineEdit()->setText( "java" );
    addArgED->setText( "" );
    enableShutdownCB->setChecked(true);
    serverTimeoutSB->setValue( 60 );
    toggleJavaControls();
}

void KJavaOptions::save()
{
    java_global_policies.save();
    m_pConfig->writeEntry( "ShowJavaConsole", javaConsoleCB->isChecked() );
    m_pConfig->writeEntry( "JavaArgs", addArgED->text() );
    m_pConfig->writePathEntry( "JavaPath", pathED->lineEdit()->text() );
    m_pConfig->writeEntry( "UseSecurityManager", javaSecurityManagerCB->isChecked() );
    m_pConfig->writeEntry( "UseKio", useKioCB->isChecked() );
    m_pConfig->writeEntry( "ShutdownAppletServer", enableShutdownCB->isChecked() );
    m_pConfig->writeEntry( "AppletServerTimeout", serverTimeoutSB->value() );

    domainSpecific->save(m_groupname,"JavaDomains");

    if (_removeJavaDomainSettings) {
      m_pConfig->deleteEntry("JavaDomainSettings");
      _removeJavaDomainSettings = false;
    }

    // sync moved to KJSParts::save
//    m_pConfig->sync();
}

void KJavaOptions::slotChanged()
{
    setChanged(true);
}


void KJavaOptions::toggleJavaControls()
{
    bool isEnabled = true; //enableJavaGloballyCB->isChecked();

    java_global_policies.setFeatureEnabled( enableJavaGloballyCB->isChecked() );
    javaConsoleCB->setEnabled( isEnabled );
    javaSecurityManagerCB->setEnabled( isEnabled );
    useKioCB->setEnabled( isEnabled );
    addArgED->setEnabled( isEnabled );
    pathED->setEnabled( isEnabled );
    enableShutdownCB->setEnabled( isEnabled );

    serverTimeoutSB->setEnabled( enableShutdownCB->isChecked() && isEnabled );
}

// == class JavaDomainListView =====

JavaDomainListView::JavaDomainListView(KConfig *config,const QString &group,
	QWidget *parent,const char *name)
	: DomainListView(config,i18n( "Doma&in-Specific" ), parent, name),
	group(group) {
}

JavaDomainListView::~JavaDomainListView() {
}

void JavaDomainListView::updateDomainListLegacy(const QStringList &domainConfig)
{
    domainSpecificLV->clear();
    JavaPolicies pol(config,group,false);
    pol.defaults();
    for ( QStringList::ConstIterator it = domainConfig.begin();
          it != domainConfig.end(); ++it)
    {
        QString domain;
        KHTMLSettings::KJavaScriptAdvice javaAdvice;
        KHTMLSettings::KJavaScriptAdvice javaScriptAdvice;
        KHTMLSettings::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
	if (javaAdvice != KHTMLSettings::KJavaScriptDunno) {
          QListViewItem* index = new QListViewItem( domainSpecificLV, domain,
                                                  i18n(KHTMLSettings::adviceToStr(javaAdvice))  );
          pol.setDomain(domain);
          pol.setFeatureEnabled(javaAdvice != KHTMLSettings::KJavaScriptReject);
          domainPolicies[index] = new JavaPolicies(pol);
	}
    }
}

void JavaDomainListView::setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
		Policies */*pol*/) {
  QString caption;
  switch (trigger) {
    case AddButton: caption = i18n( "New Java Policy" ); break;
    case ChangeButton: caption = i18n( "Change Java Policy" ); break;
    default: ; // inhibit gcc warning
  }/*end switch*/
  pDlg.setCaption(caption);
  pDlg.setFeatureEnabledLabel(i18n("&Java policy:"));
  pDlg.setFeatureEnabledWhatsThis(i18n("Select a Java policy for "
                                    "the above host or domain."));
  pDlg.refresh();
}

JavaPolicies *JavaDomainListView::createPolicies() {
  return new JavaPolicies(config,group,false);
}

JavaPolicies *JavaDomainListView::copyPolicies(Policies *pol) {
  return new JavaPolicies(*static_cast<JavaPolicies *>(pol));
}

#include "javaopts.moc"
