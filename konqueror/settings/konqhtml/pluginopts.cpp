// (c) 2002 Leo Savernik, per-domain settings
// (c) 2001, Daniel Naber, based on javaopts.cpp
// (c) 2000 Stefan Schimanski <1Stein@gmx.de>, Netscape parts

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <qregexp.h>
#include <qlayout.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kurlrequester.h>

#include <qwhatsthis.h>
#include <qvgroupbox.h>
#include <kdebug.h>
#include <X11/Xlib.h>

#include <kapplication.h>
#include <dcopclient.h>
#include <kprocio.h>
#include <qprogressdialog.h>
#include <kmessagebox.h>
#include <klistview.h>

#include "htmlopts.h"
#include "pluginopts.h"
#include "policydlg.h"


#include "pluginopts.moc"

// == class PluginPolicies =====

PluginPolicies::PluginPolicies(KConfig* config, const QString &group, bool global,
  		const QString &domain) :
	Policies(config,group,global,domain,"plugins.","EnablePlugins") {
}

PluginPolicies::~PluginPolicies() {
}

// == class KPluginOptions =====

KPluginOptions::KPluginOptions( KConfig* config, QString group, QWidget *parent,
                            const char *)
    : KCModule( parent, "kcmkonqhtml" ),
      m_pConfig( config ),
      m_groupname( group ),
      global_policies(config,group,true)
{
    QVBoxLayout* toplevel = new QVBoxLayout( this, 0, KDialog::spacingHint() );

    /***************************************************************************
     ********************* Global Settings *************************************
     **************************************************************************/
    QVGroupBox* globalGB = new QVGroupBox( i18n( "Global Settings" ), this );
    toplevel->addWidget( globalGB );
    enablePluginsGloballyCB = new QCheckBox( i18n( "&Enable plugins globally" ), globalGB );
    connect( enablePluginsGloballyCB, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );
    connect( enablePluginsGloballyCB, SIGNAL( clicked() ), this, SLOT( slotTogglePluginsEnabled() ) );

    QFrame *hrule = new QFrame(globalGB);
    hrule->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    hrule->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed);

    /***************************************************************************
     ********************* Domain-specific Settings ****************************
     **************************************************************************/
    QPushButton *domainSpecPB = new QPushButton(i18n("Domain-Specific Settin&gs..."),
    						globalGB);
    domainSpecPB->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    connect(domainSpecPB,SIGNAL(clicked()),SLOT(slotShowDomainDlg()));

    domainSpecificDlg = new KDialogBase(KDialogBase::Swallow,
    			i18n("Domain-Specific Policies"),KDialogBase::Close,
			KDialogBase::Close,this,"domainSpecificDlg", true);

    domainSpecific = new PluginDomainListView(config,group,domainSpecificDlg);
    domainSpecific->setMinimumSize(320,200);
    connect(domainSpecific,SIGNAL(changed(bool)),SLOT(slotChanged()));

    domainSpecificDlg->setMainWidget(domainSpecific);

    /***************************************************************************
     ********************** WhatsThis? items ***********************************
     **************************************************************************/
    QWhatsThis::add( enablePluginsGloballyCB, i18n("Enables the execution of plugins "
          "that can be contained in HTML pages, e.g. Macromedia Flash. "
          "Note that, as with any browser, enabling active contents can be a security problem.") );

    QString wtstr = i18n("This box contains the domains and hosts you have set "
                         "a specific plugin policy for. This policy will be used "
                         "instead of the default policy for enabling or disabling plugins on pages sent by these "
                         "domains or hosts. <p>Select a policy and use the controls on "
                         "the right to modify it.");
    QWhatsThis::add( domainSpecific->listView(), wtstr );
    QWhatsThis::add( domainSpecific->importButton(), i18n("Click this button to choose the file that contains "
                                          "the plugin policies. These policies will be merged "
                                          "with the existing ones. Duplicate entries are ignored.") );
    QWhatsThis::add( domainSpecific->exportButton(), i18n("Click this button to save the plugin policy to a zipped "
                                          "file. The file, named <b>plugin_policy.tgz</b>, will be "
                                          "saved to a location of your choice." ) );
    QWhatsThis::add( domainSpecific, i18n("Here you can set specific plugin policies for any particular "
                                            "host or domain. To add a new policy, simply click the <i>New...</i> "
                                            "button and supply the necessary information requested by the "
                                            "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                            "button and choose the new policy from the policy dialog box. Clicking "
                                            "on the <i>Delete</i> button will remove the selected policy causing the default "
                                            "policy setting to be used for that domain.") );
#if 0
                                            "The <i>Import</i> and <i>Export</i> "
                                            "button allows you to easily share your policies with other people by allowing "
                                            "you to save and retrieve them from a zipped file.") );
#endif

/***********************************************************************************/

    QVGroupBox* netscapeGB = new QVGroupBox( i18n( "Netscape Plugins" ), this );
    toplevel->addWidget( netscapeGB );

    // create Designer made widget
    m_widget = new NSConfigWidget( netscapeGB, "configwidget" );

    // setup widgets
    connect( m_widget->scanAtStartup, SIGNAL(clicked()), SLOT(change()) );
    connect( m_widget->scanButton, SIGNAL(clicked()), SLOT(scan()) );

    m_changed = false;

    dirInit();
    pluginInit();

    // Finally do the loading
    load();
}

KPluginOptions::~KPluginOptions()
{
  delete m_pConfig;
}

void KPluginOptions::load()
{
    // *** load ***
    global_policies.load();
    bool bPluginGlobal = global_policies.isFeatureEnabled();

    // *** apply to GUI ***
    enablePluginsGloballyCB->setChecked( bPluginGlobal );

    domainSpecific->initialize(m_pConfig->readListEntry("PluginDomains"));

/***********************************************************************************/

  KConfig *config = new KConfig("kcmnspluginrc", true);

  config->setGroup("Misc");
  m_widget->scanAtStartup->setChecked( config->readBoolEntry( "startkdeScan", false ) );

  dirLoad( config );
  pluginLoad( config );

  delete config;

  change( false );
}

void KPluginOptions::defaults()
{
    global_policies.defaults();
    enablePluginsGloballyCB->setChecked( global_policies.isFeatureEnabled() );

/***********************************************************************************/

    KConfig *config = new KConfig( QString::null, true, false );

    m_widget->scanAtStartup->setChecked( false );

    m_widget->dirEdit->setURL("");
    m_widget->dirEdit->setEnabled( false );
    m_widget->dirRemove->setEnabled( false );

    dirLoad( config );
    pluginLoad( config );

    delete config;

    change();
}

void KPluginOptions::save()
{
    global_policies.save();

    domainSpecific->save(m_groupname,"PluginDomains");

    m_pConfig->sync();	// I need a sync here, otherwise "apply" won't work
    			// instantly

  QByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );

/***********************************************************************************/

    KConfig *config= new KConfig("kcmnspluginrc", false);

    dirSave( config );
    pluginSave( config );

    config->setGroup("Misc");
    config->writeEntry( "startkdeScan", m_widget->scanAtStartup->isChecked() );
    config->sync();
    delete config;

    change( false );
}

QString KPluginOptions::quickHelp() const
{
      return i18n("<h1>Konqueror Plugins</h1> The Konqueror web browser can use Netscape"
        " plugins to show special content, just like the Navigator does. Please note that"
        " the way you have to install Netscape plugins may depend on your distribution. A typical"
        " place to install them is, for example, '/opt/netscape/plugins'.");
}

void KPluginOptions::slotChanged()
{
    emit changed(true);
}

void KPluginOptions::slotTogglePluginsEnabled() {
  global_policies.setFeatureEnabled(enablePluginsGloballyCB->isChecked());
}

void KPluginOptions::slotShowDomainDlg() {
  domainSpecificDlg->show();
}

/***********************************************************************************/

void KPluginOptions::scan()
{
    if ( m_changed ) {
        int ret = KMessageBox::warningYesNoCancel( this,
                                                    i18n("Do you want to apply your changes "
                                                         "before the scan? Otherwise the "
                                                         "changes will be lost.") );
        if ( ret==KMessageBox::Cancel ) return;
        if ( ret==KMessageBox::Yes )
             save();
    }

    KProcIO* nspluginscan = new KProcIO;
    QString scanExe = KGlobal::dirs()->findExe("nspluginscan");
    if (!scanExe) {
        kdDebug() << "can't find nspluginviewer" << endl;
        delete nspluginscan;

        KMessageBox::sorry ( this,
                             i18n("The nspluginscan executable cannot be found. "
                                  "Netscape plugins will not be scanned.") );
        return;
    }

    // find nspluginscan executable
    m_progress = new QProgressDialog( i18n("Scanning for plugins"), i18n("Cancel"), 100, this );
    m_progress->setProgress( 5 );

    // start nspluginscan
    *nspluginscan << scanExe;
    kdDebug() << "Running nspluginscan" << endl;
    connect(nspluginscan, SIGNAL(readReady(KProcIO*)),
            this, SLOT(progress(KProcIO*)));
    connect(nspluginscan, SIGNAL(processExited(KProcess *)),
            this, SLOT(scanDone()));
    connect(m_progress, SIGNAL(cancelled()), this, SLOT(scanDone()));

    if (nspluginscan->start())
       kapp->enter_loop();

    delete nspluginscan;

    // update dialog
    m_progress->setProgress(100);
    load();
    delete m_progress;
    m_progress = 0;
}

void KPluginOptions::progress(KProcIO *proc)
{
    QString line;
    while(proc->readln(line) > 0)
        ;
    m_progress->setProgress(line.stripWhiteSpace().toInt());
}

void KPluginOptions::scanDone()
{
    kapp->exit_loop();
}


extern "C"
{
    void init_nsplugin()
    {
        KConfig *config = new KConfig("kcmnspluginrc", true);
        config->setGroup("Misc");
        bool scan = config->readBoolEntry( "startkdeScan", false );
        bool firstTime = config->readBoolEntry( "firstTime", true );
        delete config;

        if ( scan || firstTime )
        {
            system( "nspluginscan" );

            config= new KConfig("kcmnspluginrc", false);
            config->setGroup("Misc");
            config->writeEntry( "firstTime", false );
            config->sync();
            delete config;
        }
    }
}


/***********************************************************************************/


void KPluginOptions::dirInit()
{
    m_widget->dirEdit->setCaption(i18n("Select Plugin Scan Directory"));
    connect( m_widget->dirNew, SIGNAL(clicked()), SLOT(dirNew()) );
    connect( m_widget->dirRemove, SIGNAL(clicked()), SLOT(dirRemove()) );
    connect( m_widget->dirUp, SIGNAL(clicked()), SLOT(dirUp()) );
    connect( m_widget->dirDown, SIGNAL(clicked()), SLOT(dirDown()) );
    connect( m_widget->useArtsdsp, SIGNAL(clicked ()),SLOT(change()));
    connect( m_widget->dirEdit,
             SIGNAL(textChanged(const QString&)),
             SLOT(dirEdited(const QString &)) );

    connect( m_widget->dirList,
             SIGNAL(executed(QListBoxItem*)),
             SLOT(dirSelect(QListBoxItem*)) );

    connect( m_widget->dirList,
             SIGNAL(selectionChanged(QListBoxItem*)),
             SLOT(dirSelect(QListBoxItem*)) );
}


void KPluginOptions::dirLoad( KConfig *config )
{
    QStringList paths;

    // read search paths
    config->setGroup("Misc");
    if ( config->hasKey( "scanPaths" ) )
        paths = config->readListEntry( "scanPaths" );
    else {
        paths.append("$HOME/.netscape/plugins");
        paths.append("/usr/local/netscape/plugins");
        paths.append("/opt/mozilla/plugins");
        paths.append("/opt/netscape/plugins");
        paths.append("/opt/netscape/communicator/plugins");
        paths.append("/usr/lib/netscape/plugins");
        paths.append("/usr/lib/netscape/plugins-libc5");
        paths.append("/usr/lib/netscape/plugins-libc6");
        paths.append("/usr/lib/mozilla/plugins");
        paths.append("$MOZILLA_HOME/plugins");
    }

    // fill list
    m_widget->dirList->clear();
    m_widget->dirList->insertStringList( paths );

    // setup other widgets
    bool useArtsdsp = config->readBoolEntry( "useArtsdsp", true );
    m_widget->useArtsdsp->setChecked( useArtsdsp );
}


void KPluginOptions::dirSave( KConfig *config )
{
    // create stringlist
    QStringList paths;
    QListBoxItem *item = m_widget->dirList->firstItem();
    for ( ; item!=0; item=item->next() )
        if ( !item->text().isEmpty() )
            paths << item->text();

    // write entry
    config->setGroup( "Misc" );
    config->writeEntry( "scanPaths", paths );
    config->writeEntry( "useArtsdsp", m_widget->useArtsdsp->isOn() );
}


void KPluginOptions::dirSelect( QListBoxItem *item )
{
    m_widget->dirEdit->setEnabled( item!=0 );
    m_widget->dirRemove->setEnabled( item!=0 );

    unsigned cur = m_widget->dirList->index(m_widget->dirList->selectedItem());
    m_widget->dirDown->setEnabled( item!=0 && cur<m_widget->dirList->count()-1 );
    m_widget->dirUp->setEnabled( item!=0 && cur>0 );
    m_widget->dirEdit->setURL( item!=0 ? item->text() : QString::null );
 }


void KPluginOptions::dirNew()
{
    m_widget->dirList->insertItem( QString::null, 0 );
    m_widget->dirList->setCurrentItem( 0 );
    dirSelect( m_widget->dirList->selectedItem() );
    m_widget->dirEdit->setURL(QString::null);
    m_widget->dirEdit->setFocus();
    change();
}


void KPluginOptions::dirRemove()
{
    m_widget->dirEdit->setURL(QString::null);
    delete m_widget->dirList->selectedItem();
    m_widget->dirRemove->setEnabled( false );
    m_widget->dirUp->setEnabled( false );
    m_widget->dirDown->setEnabled( false );
    m_widget->dirEdit->setEnabled( false );
    change();
}


void KPluginOptions::dirUp()
{
    unsigned cur = m_widget->dirList->index(m_widget->dirList->selectedItem());
    if ( cur>0 ) {
        QString txt = m_widget->dirList->text(cur-1);
        m_widget->dirList->removeItem( cur-1 );
        m_widget->dirList->insertItem( txt, cur );

        m_widget->dirUp->setEnabled( cur-1>0 );
        m_widget->dirDown->setEnabled( true );
        change();
    }
}


void KPluginOptions::dirDown()
{
    unsigned cur = m_widget->dirList->index(m_widget->dirList->selectedItem());
    if ( cur < m_widget->dirList->count()-1 ) {
        QString txt = m_widget->dirList->text(cur+1);
        m_widget->dirList->removeItem( cur+1 );
        m_widget->dirList->insertItem( txt, cur );

        m_widget->dirUp->setEnabled( true );
        m_widget->dirDown->setEnabled( cur+1<m_widget->dirList->count()-1 );
        change();
    }
}


void KPluginOptions::dirEdited(const QString &txt )
{
    if ( m_widget->dirList->currentText()!=txt ) {
        m_widget->dirList->changeItem( txt, m_widget->dirList->currentItem() );
        change();
    }
}


/***********************************************************************************/


void KPluginOptions::pluginInit()
{
}


void KPluginOptions::pluginLoad( KConfig */*config*/ )
{
    kdDebug() << "-> KPluginOptions::fillPluginList" << endl;
    m_widget->pluginList->clear();
    QRegExp version(";version=[^:]*:");

    // open the cache file
    QFile cachef( locate("data", "nsplugins/cache") );
    if ( !cachef.exists() || !cachef.open(IO_ReadOnly) ) {
        kdDebug() << "Could not load plugin cache file!" << endl;
        return;
    }

    QTextStream cache(&cachef);

    // root object
    QListViewItem *root = new QListViewItem( m_widget->pluginList, i18n("Netscape Plugins") );
    root->setOpen( true );
    root->setSelectable( false );
    root->setExpandable( true );
    root->setPixmap(0, SmallIcon("netscape"));

    // read in cache
    QString line, plugin;
    QListViewItem *next = 0;
    QListViewItem *lastMIME = 0;
    while ( !cache.atEnd() ) {

        line = cache.readLine();
        kdDebug() << line << endl;
        if (line.isEmpty() || (line.left(1) == "#"))
            continue;

        if (line.left(1) == "[") {

            plugin = line.mid(1,line.length()-2);
            kdDebug() << "plugin=" << plugin << endl;

            // add plugin root item
            next = new QListViewItem( root, i18n("Plugin"), plugin );
            next->setOpen( true );
            next->setSelectable( false );
            next->setExpandable( true );

            lastMIME = 0;

            continue;
        }

        QStringList desc = QStringList::split(':', line, TRUE);
        QString mime = desc[0].stripWhiteSpace();
        QString name = desc[2];
        QString suffixes = desc[1];

        if (!mime.isEmpty()) {
            kdDebug() << "mime=" << mime << " desc=" << name << " suffix=" << suffixes << endl;
            lastMIME = new QListViewItem( next, lastMIME, i18n("MIME type"), mime );
            lastMIME->setOpen( true );
            lastMIME->setSelectable( false );
            lastMIME->setExpandable( true );

            QListViewItem *last = new QListViewItem( lastMIME, 0, i18n("Description"), name );
            last->setOpen( false );
            last->setSelectable( false );
            last->setExpandable( false );

            last = new QListViewItem( lastMIME, last, i18n("Suffixes"), suffixes );
            last->setOpen( false );
            last->setSelectable( false );
            last->setExpandable( false );
        }
    }

    kdDebug() << "<- KPluginOptions::fillPluginList" << endl;
}


void KPluginOptions::pluginSave( KConfig */*config*/ )
{

}

// == class PluginDomainDialog =====

PluginDomainDialog::PluginDomainDialog(QWidget *parent) :
	QWidget(parent,"PluginDomainDialog") {
  setCaption(i18n("Domain-Specific Policies"));

  thisLayout = new QVBoxLayout(this);
  thisLayout->addSpacing(6);
  QFrame *hrule = new QFrame(this);
  hrule->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  thisLayout->addWidget(hrule);
  thisLayout->addSpacing(6);

  QBoxLayout *hl = new QHBoxLayout(this,0,6);
  hl->addStretch(10);

  QPushButton *closePB = new QPushButton(i18n("&Close"),this);
  connect(closePB,SIGNAL(clicked()),SLOT(slotClose()));
  hl->addWidget(closePB);
  thisLayout->addLayout(hl);
}

PluginDomainDialog::~PluginDomainDialog() {
}

void PluginDomainDialog::setMainWidget(QWidget *widget) {
  thisLayout->insertWidget(0,widget);
}

void PluginDomainDialog::slotClose() {
  hide();
}

// == class PluginDomainListView =====

PluginDomainListView::PluginDomainListView(KConfig *config,const QString &group,
	QWidget *parent,const char *name)
	: DomainListView(config,i18n( "Doma&in-Specific" ), parent, name),
	group(group) {
}

PluginDomainListView::~PluginDomainListView() {
}

void PluginDomainListView::setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
		Policies */*pol*/) {
  QString caption;
  switch (trigger) {
    case AddButton: caption = i18n( "New Plugin Policy" ); break;
    case ChangeButton: caption = i18n( "Change Plugin Policy" ); break;
    default: ; // inhibit gcc warning
  }/*end switch*/
  pDlg.setCaption(caption);
  pDlg.setFeatureEnabledLabel(i18n("&Plugin policy:"));
  pDlg.setFeatureEnabledWhatsThis(i18n("Select a plugin policy for "
                                    "the above host or domain."));
  pDlg.refresh();
}

PluginPolicies *PluginDomainListView::createPolicies() {
  return new PluginPolicies(config,group,false);
}

PluginPolicies *PluginDomainListView::copyPolicies(Policies *pol) {
  return new PluginPolicies(*static_cast<PluginPolicies *>(pol));
}

