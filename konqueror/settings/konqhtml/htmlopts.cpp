////
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998
// (c) 2001 Waldo Bastian <bastian@kde.org>

#include <qlayout.h>//CT - 12Nov1998
#include <qwhatsthis.h>
#include <qvgroupbox.h>
#include <qlabel.h>

#include "htmlopts.h"

#include <konq_defaults.h> // include default values directly from konqueror
#include <kglobalsettings.h> // get default for DEFAULT_CHANGECURSOR
#include <klocale.h>
#include <kdialog.h>
#include <knuminput.h>
#include <kseparator.h>

#include <kapplication.h>
#include <dcopclient.h>


#include "htmlopts.moc"

enum UnderlineLinkType { UnderlineAlways=0, UnderlineNever=1, UnderlineHover=2 };
enum AnimationsType { AnimationsAlways=0, AnimationsNever=1, AnimationsLoopOnce=2 };
//-----------------------------------------------------------------------------

KMiscHTMLOptions::KMiscHTMLOptions(KConfig *config, QString group, QWidget *parent, const char *)
    : KCModule( parent, "kcmkonqhtml" ), m_pConfig(config), m_groupname(group)
{
    int row = 0;
    QGridLayout *lay = new QGridLayout(this, 10, 2, KDialog::marginHint(), KDialog::spacingHint());

     // Form completion

    QVGroupBox *bgForm = new QVGroupBox( i18n("Form Com&pletion"), this );
    m_pFormCompletionCheckBox = new QCheckBox(i18n( "Enable completion of &forms" ), bgForm);
    QWhatsThis::add( m_pFormCompletionCheckBox, i18n( "If this box is checked, Konqueror will remember"
                                                        " the data you enter in web forms and suggest it in similar fields for all forms." ) );
    connect(m_pFormCompletionCheckBox, SIGNAL(clicked()), this, SLOT(slotChanged()));

    m_pMaxFormCompletionItems = new KIntNumInput( bgForm );
    m_pMaxFormCompletionItems->setLabel( i18n( "&Maximum completions:" ) );
    m_pMaxFormCompletionItems->setRange( 1, 100 );
    QWhatsThis::add( m_pMaxFormCompletionItems,
        i18n( "Here you can select how many values Konqueror will remember for a form field." ) );
    connect(m_pMaxFormCompletionItems, SIGNAL(valueChanged(int)), SLOT(slotChanged()));

    lay->addMultiCellWidget( bgForm, row, row, 0, 1 );
    row++;


    // Misc

    cbCursor = new QCheckBox(i18n("Chan&ge cursor over links"), this);
    lay->addMultiCellWidget(cbCursor, row, row, 0, 1);
    row++;

    QWhatsThis::add( cbCursor, i18n("If this option is set, the shape of the cursor will change "
       "(usually to a hand) if it is moved over a hyperlink.") );

    connect(cbCursor, SIGNAL(clicked()), this, SLOT(slotChanged()));

    m_pShowMMBInTabs = new QCheckBox( i18n( "Open &links in new tab instead of in new window" ), this );
    QWhatsThis::add( m_pShowMMBInTabs, i18n("This will open a new tab instead of a new window in various situations, "
                          "such as choosing a link or a folder with the middle mouse button.") );
    lay->addMultiCellWidget( m_pShowMMBInTabs, row, row, 0, 1);
    row++;
    connect(m_pShowMMBInTabs, SIGNAL(clicked()), this, SLOT(slotChanged()));

    m_pNewTabsInFront = new QCheckBox( i18n( "Au&tomatically activate new opened tabs" ), this );
    QWhatsThis::add( m_pNewTabsInFront, i18n("This will open a new tab in front otherwise as background tab.") );
    lay->addMultiCellWidget( m_pNewTabsInFront, row, row, 0, 1);
    row++;
    connect(m_pNewTabsInFront, SIGNAL(clicked()), this, SLOT(slotChanged()));

    m_pOpenAfterCurrentPage = new QCheckBox( i18n( "Open links after current page" ), this );
    lay->addMultiCellWidget( m_pOpenAfterCurrentPage, row, row, 0, 1);
    row++;
    connect(m_pOpenAfterCurrentPage, SIGNAL(clicked()), this, SLOT(slotChanged()));



    m_pTabConfirm = new QCheckBox( i18n( "Confirm &when closing windows with multiple tabs" ), this );
    QWhatsThis::add( m_pTabConfirm, i18n("This will ask you whether you are sure you want to close "
                          "a window when it has multiple tabs opened in it.") );
    lay->addMultiCellWidget( m_pTabConfirm, row, row, 0, 1);
    row++;
    connect(m_pTabConfirm, SIGNAL(clicked()), this, SLOT(slotChanged()));

    m_pBackRightClick = new QCheckBox( i18n( "Right click goes &back in history" ), this );
    QWhatsThis::add( m_pBackRightClick, i18n(
      "If this box is checked, you can go back in history by right clicking on a Konqueror view. "
      "To access the context menu, press the right mouse button and move." ) );
    lay->addMultiCellWidget( m_pBackRightClick, row, row, 0, 1);
    row++;
    connect(m_pBackRightClick, SIGNAL(clicked()), this, SLOT(slotChanged()));

    m_pAutoLoadImagesCheckBox = new QCheckBox( i18n( "A&utomatically load images"), this );
    QWhatsThis::add( m_pAutoLoadImagesCheckBox, i18n( "If this box is checked, Konqueror will automatically load any images that are embedded in a web page. Otherwise, it will display placeholders for the images, and you can then manually load the images by clicking on the image button.<br>Unless you have a very slow network connection, you will probably want to check this box to enhance your browsing experience." ) );
    connect(m_pAutoLoadImagesCheckBox, SIGNAL(clicked()), this, SLOT(slotChanged()));
    lay->addMultiCellWidget( m_pAutoLoadImagesCheckBox, row, row, 0, 1 );
    row++;

    m_pAutoRedirectCheckBox = new QCheckBox( i18n( "Allow automatic delayed &reloading/redirecting"), this );
    QWhatsThis::add( m_pAutoRedirectCheckBox,
    i18n( "Some web pages request an automatic reload or redirection after a certain period of time. By unchecking this box Konqueror will ignore these requests." ) );
    connect(m_pAutoRedirectCheckBox, SIGNAL(clicked()), this, SLOT(slotChanged()));
    lay->addMultiCellWidget( m_pAutoRedirectCheckBox, row, row, 0, 1 );
    row++;


    // More misc

    KSeparator *sep = new KSeparator(this);
    lay->addMultiCellWidget(sep, row, row, 0, 1);
    row++;

    QLabel *label = new QLabel( i18n("Und&erline links:"), this);
    m_pUnderlineCombo = new QComboBox( false, this );
    label->setBuddy(m_pUnderlineCombo);
    m_pUnderlineCombo->insertItem(i18n("Enabled"), UnderlineAlways);
    m_pUnderlineCombo->insertItem(i18n("Disabled"), UnderlineNever);
    m_pUnderlineCombo->insertItem(i18n("Only on Hover"), UnderlineHover);
    lay->addWidget(label, row, 0);
    lay->addWidget(m_pUnderlineCombo, row, 1);
    row++;
    QString whatsThis = i18n("Controls how Konqueror handles underlining hyperlinks:<br>"
	    "<ul><li><b>Enabled</b>: Always underline links</li>"
	    "<li><b>Disabled</b>: Never underline links</li>"
	    "<li><b>Only on Hover</b>: Underline when the mouse is moved over the link</li>"
	    "</ul><br><i>Note: The site's CSS definitions can override this value</i>");
    QWhatsThis::add( label, whatsThis);
    QWhatsThis::add( m_pUnderlineCombo, whatsThis);
    connect(m_pUnderlineCombo, SIGNAL(activated(int)), this, SLOT(slotChanged()));



    label = new QLabel( i18n("A&nimations:"), this);
    m_pAnimationsCombo = new QComboBox( false, this );
    label->setBuddy(m_pAnimationsCombo);
    m_pAnimationsCombo->insertItem(i18n("Enabled"), AnimationsAlways);
    m_pAnimationsCombo->insertItem(i18n("Disabled"), AnimationsNever);
    m_pAnimationsCombo->insertItem(i18n("Show Only Once"), AnimationsLoopOnce);
    lay->addWidget(label, row, 0);
    lay->addWidget(m_pAnimationsCombo, row, 1);
    row++;
    whatsThis = i18n("Controls how Konqueror shows animated images:<br>"
	    "<ul><li><b>Enabled</b>: Show all animations completely.</li>"
	    "<li><b>Disabled</b>: Never show animations, show the start image only.</li>"
	    "<li><b>Show only once</b>: Show all animations completely but do not repeat them.</li>");
    QWhatsThis::add( label, whatsThis);
    QWhatsThis::add( m_pAnimationsCombo, whatsThis);
    connect(m_pAnimationsCombo, SIGNAL(activated(int)), this, SLOT(slotChanged()));

    lay->setRowStretch(row, 1);

    load();
}

KMiscHTMLOptions::~KMiscHTMLOptions()
{
    delete m_pConfig;
}

void KMiscHTMLOptions::load()
{
    // *** load ***
    m_pConfig->setGroup( "MainView Settings" );
    bool bBackRightClick = m_pConfig->readBoolEntry( "BackRightClick", false );
    m_pConfig->setGroup( "HTML Settings" );
    bool changeCursor = m_pConfig->readBoolEntry("ChangeCursor", KDE_DEFAULT_CHANGECURSOR);
    bool underlineLinks = m_pConfig->readBoolEntry("UnderlineLinks", DEFAULT_UNDERLINELINKS);
    bool hoverLinks = m_pConfig->readBoolEntry("HoverLinks", true);
    bool bAutoLoadImages = m_pConfig->readBoolEntry( "AutoLoadImages", true );
    bool bAutoRedirect = m_pConfig->readBoolEntry( "AutoDelayedActions", true );
    QString strAnimations = m_pConfig->readEntry( "ShowAnimations" ).lower();

    // *** apply to GUI ***
    cbCursor->setChecked( changeCursor );
    m_pAutoLoadImagesCheckBox->setChecked( bAutoLoadImages );
    m_pAutoRedirectCheckBox->setChecked( bAutoRedirect );
    m_pBackRightClick->setChecked( bBackRightClick );

    // we use two keys for link underlining so that this config file
    // is backwards compatible with KDE 2.0.  the HoverLink setting
    // has precedence over the UnderlineLinks setting
    if (hoverLinks)
    {
        m_pUnderlineCombo->setCurrentItem( UnderlineHover );
    }
    else
    {
        if (underlineLinks)
            m_pUnderlineCombo->setCurrentItem( UnderlineAlways );
        else
            m_pUnderlineCombo->setCurrentItem( UnderlineNever );
    }
    if (strAnimations == "disabled")
       m_pAnimationsCombo->setCurrentItem( AnimationsNever );
    else if (strAnimations == "looponce")
       m_pAnimationsCombo->setCurrentItem( AnimationsLoopOnce );
    else
       m_pAnimationsCombo->setCurrentItem( AnimationsAlways );

    m_pFormCompletionCheckBox->setChecked( m_pConfig->readBoolEntry( "FormCompletion", true ) );
    m_pMaxFormCompletionItems->setValue( m_pConfig->readNumEntry( "MaxFormCompletionItems", 10 ) );
    m_pMaxFormCompletionItems->setEnabled( m_pFormCompletionCheckBox->isChecked() );

    m_pConfig->setGroup("FMSettings");
    m_pShowMMBInTabs->setChecked( m_pConfig->readBoolEntry( "MMBOpensTab", false ) );
    m_pNewTabsInFront->setChecked( m_pConfig->readBoolEntry( "NewTabsInFront", true ) );
    m_pOpenAfterCurrentPage->setChecked( m_pConfig->readBoolEntry( "OpenAfterCurrentPage", false ) );

    m_pConfig->setGroup("Notification Messages");
    m_pTabConfirm->setChecked( !m_pConfig->hasKey("MultipleTabConfirm") );
}

void KMiscHTMLOptions::defaults()
{
    cbCursor->setChecked( false );
    m_pAutoLoadImagesCheckBox->setChecked( true );
    m_pAutoRedirectCheckBox->setChecked( true );
    m_pUnderlineCombo->setCurrentItem( UnderlineAlways );
    m_pAnimationsCombo->setCurrentItem( AnimationsAlways );
    m_pFormCompletionCheckBox->setChecked(true);
    m_pMaxFormCompletionItems->setEnabled( true );
    m_pShowMMBInTabs->setChecked( false );
    m_pNewTabsInFront->setChecked( true );
    m_pTabConfirm->setChecked( true );
    m_pBackRightClick->setChecked( false );
    m_pMaxFormCompletionItems->setValue( 10 );
}

void KMiscHTMLOptions::save()
{
    m_pConfig->setGroup( "MainView Settings" );
    m_pConfig->writeEntry( "BackRightClick", m_pBackRightClick->isChecked() );
    m_pConfig->setGroup( "HTML Settings" );
    m_pConfig->writeEntry( "ChangeCursor", cbCursor->isChecked() );
    m_pConfig->writeEntry( "AutoLoadImages", m_pAutoLoadImagesCheckBox->isChecked() );
    m_pConfig->writeEntry( "AutoDelayedActions", m_pAutoRedirectCheckBox->isChecked() );
    switch(m_pUnderlineCombo->currentItem())
    {
      case UnderlineAlways:
        m_pConfig->writeEntry( "UnderlineLinks", true );
        m_pConfig->writeEntry( "HoverLinks", false );
        break;
      case UnderlineNever:
        m_pConfig->writeEntry( "UnderlineLinks", false );
        m_pConfig->writeEntry( "HoverLinks", false );
        break;
      case UnderlineHover:
        m_pConfig->writeEntry( "UnderlineLinks", false );
        m_pConfig->writeEntry( "HoverLinks", true );
        break;
    }
    switch(m_pAnimationsCombo->currentItem())
    {
      case AnimationsAlways:
        m_pConfig->writeEntry( "ShowAnimations", "Enabled" );
        break;
      case AnimationsNever:
        m_pConfig->writeEntry( "ShowAnimations", "Disabled" );
        break;
      case AnimationsLoopOnce:
        m_pConfig->writeEntry( "ShowAnimations", "LoopOnce" );
        break;
    }

    m_pConfig->writeEntry( "FormCompletion", m_pFormCompletionCheckBox->isChecked() );
    m_pConfig->writeEntry( "MaxFormCompletionItems", m_pMaxFormCompletionItems->value() );

    m_pConfig->setGroup("FMSettings");
    m_pConfig->writeEntry( "MMBOpensTab", m_pShowMMBInTabs->isChecked() );
    m_pConfig->writeEntry( "NewTabsInFront", m_pNewTabsInFront->isChecked() );
    m_pConfig->writeEntry( "OpenAfterCurrentPage", m_pOpenAfterCurrentPage->isChecked() );

    // It only matters wether the key is present, its value has no meaning
    m_pConfig->setGroup("Notification Messages");
    if ( m_pTabConfirm->isChecked() ) m_pConfig->deleteEntry( "MultipleTabConfirm" );
    else m_pConfig->writeEntry( "MultipleTabConfirm", true );

    m_pConfig->sync();

  QByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );

}


void KMiscHTMLOptions::slotChanged()
{
    m_pMaxFormCompletionItems->setEnabled( m_pFormCompletionCheckBox->isChecked() );
    emit changed(true);
}

QString KMiscHTMLOptions::quickHelp() const
{
  return i18n("<h1>Konqueror Browser</h1> Here you can configure Konqueror's browser "
              "functionality. Please note that the file manager "
              "functionality has to be configured using the \"File Manager\" "
              "configuration module. You can make some "
              "settings how Konqueror should handle the HTML code in "
              "the web pages it loads. It is usually not necessary to "
              "change anything here.");
}
