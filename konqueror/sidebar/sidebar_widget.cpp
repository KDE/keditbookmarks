/***************************************************************************
                               sidebar_widget.cpp
                             -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    copyright            : (C) 2001 Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <config.h>

#include <limits.h>

#include <qdir.h>
#include <qpopupmenu.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <qlayout.h>

#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kicondialog.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <konq_events.h>
#include <kfileitem.h>
#include <kio/netaccess.h>
#include <kpopupmenu.h>
#include <kprocess.h>

#include "konqsidebar.h"

#include "sidebar_widget.h"
#include "sidebar_widget.moc"

addBackEnd::addBackEnd(QObject *parent,class QPopupMenu *addmenu,const char *name):QObject(parent,name)
{

	menu = addmenu;
	connect(menu,SIGNAL(aboutToShow()),this,SLOT(aboutToShowAddMenu()));
	connect(menu,SIGNAL(activated(int)),this,SLOT(activatedAddMenu(int)));
}

void addBackEnd::aboutToShowAddMenu()
{
	if (!menu)
		return;
	KStandardDirs *dirs = KGlobal::dirs();
	QStringList list = dirs->findAllResources("data","konqsidebartng/add/*.desktop",true,true);
	libNames.setAutoDelete(true);
	libNames.resize(0);
	libParam.setAutoDelete(true);
	libParam.resize(0);
	menu->clear();
	int i = 0;

	for (QStringList::Iterator it = list.begin(); it != list.end(); ++it, i++ )
	{
		KSimpleConfig *confFile;

		confFile = new KSimpleConfig(*it, true);
		confFile->setGroup("Desktop Entry");
		QString icon = confFile->readEntry("Icon","");
		if (!icon.isEmpty())
		{
			menu->insertItem(SmallIcon(icon),
					 confFile->readEntry("Name",""), i);
		} else {
			menu->insertItem(confFile->readEntry("Name",""), i);
		}
		libNames.resize(libNames.size()+1);
		libNames.insert(libNames.count(), new QString(confFile->readEntry("X-KDE-KonqSidebarAddModule","")));
		libParam.resize(libParam.size()+1);
		libParam.insert(libParam.count(), new QString(confFile->readEntry("X-KDE-KonqSidebarAddParam","")));
		delete confFile;
	}
	menu->insertSeparator();
	menu->insertItem(i18n("Rollback to System Default"), i);
}


void addBackEnd::doRollBack()
{
	if (KMessageBox::questionYesNo(0,i18n("<qt>This removes all your entries from the sidebar and adds the system default ones.<BR><B>This procedure is irreversible</B><BR>Do you want to proceed?</qt>"))==KMessageBox::Yes)
	{
		KStandardDirs *dirs = KGlobal::dirs();
		QString loc=dirs->saveLocation("data","konqsidebartng/",true);
		QDir dir(loc);
		QStringList dirEntries = dir.entryList( QDir::Dirs | QDir::NoSymLinks );
		dirEntries.remove(".");
		dirEntries.remove("..");
		for ( QStringList::Iterator it = dirEntries.begin(); it != dirEntries.end(); ++it ) {
			if ((*it)!="add")
				 KIO::NetAccess::del(loc+(*it));
		}
		emit initialCopyNeeded();
	}
}


static QString findFileName(const QString* tmpl) {
	QString myFile, filename;
	KStandardDirs *dirs = KGlobal::dirs();
	QString tmp = *tmpl;

	dirs->saveLocation("data", "konqsidebartng/entries/", true);
	tmp.prepend("/konqsidebartng/entries/");
	filename = tmp.arg("");
	myFile = locateLocal("data", filename);

	if (QFile::exists(myFile)) {
		for (ulong l = 0; l < ULONG_MAX; l++) {
			filename = tmp.arg(l);
			myFile = locateLocal("data", filename);
			if (!QFile::exists(myFile)) {
				break;
			} else {
				myFile = QString::null;
			}
		}
	}

	return myFile;
}

void addBackEnd::activatedAddMenu(int id)
{
	kdDebug() << "activatedAddMenu: " << QString("%1").arg(id) << endl;
	if (((uint)id) == libNames.size())
		doRollBack();
	if(((uint)id) >= libNames.size())
		return;

	KLibLoader *loader = KLibLoader::self();

        // try to load the library
	QString libname = *libNames.at(id);
        KLibrary *lib = loader->library(QFile::encodeName(libname));
        if (lib)
       	{
		// get the create_ function
		QString factory("add_");
		factory = factory+(*libNames.at(id));
		void *add = lib->symbol(QFile::encodeName(factory));

		if (add)
		{
			//call the add function
			bool (*func)(QString*, QString*, QMap<QString,QString> *);
			QMap<QString,QString> map;
			func = (bool (*)(QString*, QString*, QMap<QString,QString> *)) add;
			QString *tmp = new QString("");
			if (func(tmp,libParam.at(id),&map))
			{
				QString myFile = findFileName(tmp);

				if (!myFile.isEmpty())
				{
					KSimpleConfig scf(myFile,false);
					scf.setGroup("Desktop Entry");
					for (QMap<QString,QString>::ConstIterator it = map.begin(); it != map.end(); ++it)
						scf.writeEntry(it.key(), it.data());
					scf.sync();
					emit updateNeeded();

				} else {
					kdWarning() << "No unique filename found" << endl;
				}
			} else {	
				kdWarning() << "No new entry (error?)" << endl;
			}
			delete tmp;
		}
	} else {
		kdWarning() << "libname:" << libNames.at(id)
			<< " doesn't specify a library!" << endl;
	}
}


/**************************************************************/
/*                      Sidebar_Widget                        */
/**************************************************************/

Sidebar_Widget::Sidebar_Widget(QWidget *parent, KParts::ReadOnlyPart *par, const char *name)
	:QWidget(parent,name), m_partParent(par)
{
	m_somethingVisible = false;
	m_initial = true;
	m_noUpdate = false;
	m_layout = 0;
	m_currentButton = 0;
	m_activeModule = 0;
        //kdDebug() << "**** Sidebar_Widget:SidebarWidget()"<<endl;
        m_path = KGlobal::dirs()->saveLocation("data", "konqsidebartng/entries/", true);
	m_buttons.setAutoDelete(true);
	m_hasStoredUrl = false;
	m_latestViewed = -1;
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

	QSplitter *splitterWidget = splitter();
	if (splitterWidget)
		splitterWidget->setResizeMode(parent, QSplitter::FollowSizeHint);

	m_area = new KDockArea(this);
	m_area->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	m_mainDockWidget = m_area->createDockWidget("free", 0);
	m_mainDockWidget->setWidget(new QWidget(m_mainDockWidget));
	m_area->setMainDockWidget(m_mainDockWidget);
	m_area->setMinimumWidth(0);
	m_mainDockWidget->setDockSite(KDockWidget::DockTop);
	m_mainDockWidget->setEnableDocking(KDockWidget::DockNone);

   	m_buttonBar = new KMultiTabBar(KMultiTabBar::Vertical,this);
	m_buttonBar->showActiveTabTexts(true);

	m_menu = new QPopupMenu(this, "Sidebar_Widget::Menu");
	QPopupMenu *addMenu = new QPopupMenu(this, "Sidebar_Widget::addPopup");
	m_menu->insertItem(i18n("Add New"), addMenu, 0);
	m_menu->insertSeparator();
	m_menu->insertItem(i18n("Multiple Views"), 1);
	m_menu->insertItem(i18n("Show Tabs Left"), 2);
	m_menu->insertItem(i18n("Show Configuration Button"), 3);
	m_menu->insertSeparator();
	m_menu->insertItem(i18n("Save Opened Views"), this, SLOT(saveOpenViews()));
	m_menu->insertSeparator();
	m_menu->insertItem(SmallIconSet("remove"), i18n("Close Navigation Panel"),
			par, SLOT(deleteLater()));
        connect(m_menu, SIGNAL(aboutToShow()),
		this, SLOT(aboutToShowConfigMenu()));
	connect(m_menu, SIGNAL(activated(int)),
		this, SLOT(activatedMenu(int)));

	m_buttonPopup = 0;
	addBackEnd *ab = new addBackEnd(this, addMenu, "Sidebar_Widget-addBackEnd");
	connect(ab, SIGNAL(updateNeeded()),
		this, SLOT(updateButtons()));
	connect(ab, SIGNAL(initialCopyNeeded()),
		this, SLOT(finishRollBack()));

	initialCopy();

	m_config = new KConfig("konqsidebartng.rc");
	connect(&m_configTimer, SIGNAL(timeout()), 
		this, SLOT(saveConfig()));
        readConfig();
	m_somethingVisible = !m_openViews.isEmpty();
	doLayout();
	QTimer::singleShot(0,this,SLOT(createButtons()));
	connect(m_area, SIGNAL(dockWidgetHasUndocked(KDockWidget*)),
		this, SLOT(dockWidgetHasUndocked(KDockWidget*)));
}

void Sidebar_Widget::addWebSideBar(const KURL& url, const QString& /*name*/) {
	//kdDebug() << "Web sidebar entry to be added: " << url.url()
	//	<< " [" << name << "]" << endl;

	// Look for existing ones with this URL
	KStandardDirs *dirs = KGlobal::dirs();
	dirs->saveLocation("data", "konqsidebartng/entries/", true);
	QString list = locateLocal("data", "/konqsidebartng/entries/");

	// Go through list to see which ones exist.  Check them for the URL
	QStringList files = QDir(list).entryList("websidebarplugin*.desktop");
	for (QStringList::Iterator it = files.begin(); it != files.end(); ++it){
		KSimpleConfig scf(list + *it, false);
		scf.setGroup("Desktop Entry");
		if (scf.readEntry("URL", QString::null) == url.url()) {
			// We already have this one!
			KMessageBox::information(0L,
					i18n("This entry already exists."));
			return;
		}
	}

	QString tmpl = "websidebarplugin%1.desktop";
	QString myFile = findFileName(&tmpl);

	if (!myFile.isEmpty()) {
		KSimpleConfig scf(myFile, false);
		scf.setGroup("Desktop Entry");
		scf.writeEntry("Type", "Link");
		scf.writeEntry("URL", url.url());
		scf.writeEntry("Icon", "netscape");
		scf.writeEntry("Name", i18n("Web SideBar Plugin"));
		scf.writeEntry("Open", "true");
		scf.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_web");
		scf.sync();

		updateButtons(); // update
	}
}


void Sidebar_Widget::finishRollBack()
{
        m_path = KGlobal::dirs()->saveLocation("data","konqsidebartng/entries/",true);
        initialCopy();
        QTimer::singleShot(0,this,SLOT(updateButtons()));
}

void Sidebar_Widget::saveOpenViews()
{
        m_config->writeEntry("OpenViews",m_visibleViews);
        m_config->sync();
}

void Sidebar_Widget::saveConfig()
{
	m_config->writeEntry("SingleWidgetMode",m_singleWidgetMode);
	m_config->writeEntry("ShowExtraButtons",m_showExtraButtons);
	m_config->writeEntry("ShowTabsLeft", m_showTabsLeft);
	m_config->writeEntry("SavedWidth",m_savedWidth);
	m_config->sync();
}

void Sidebar_Widget::doLayout()
{
	if (m_layout)
		delete m_layout;

	m_layout = new QHBoxLayout(this);
	if  (m_showTabsLeft)
	{
		m_layout->add(m_buttonBar);
		m_layout->add(m_area);
		m_buttonBar->setPosition(KMultiTabBar::Left);
	} else {
		m_layout->add(m_area);
		m_layout->add(m_buttonBar);
		m_buttonBar->setPosition(KMultiTabBar::Right);
	}
	m_layout->activate();
}


void Sidebar_Widget::aboutToShowConfigMenu()
{
	m_menu->setItemChecked(1, !m_singleWidgetMode);
	m_menu->setItemChecked(2, m_showTabsLeft);
	m_menu->setItemChecked(3, m_showExtraButtons);
}


void Sidebar_Widget::initialCopy()
{
	// kdDebug()<<"Initial copy"<<endl;
	QString dirtree_dir = KGlobal::dirs()->findDirs("data","konqsidebartng/entries/").last();

	if (dirtree_dir == m_path)
		return; // oups?

        if ( !dirtree_dir.isEmpty() && dirtree_dir != m_path )
        {
		KSimpleConfig gcfg(dirtree_dir+".version");
		KSimpleConfig lcfg(m_path+".version");
		int gversion = gcfg.readNumEntry("Version", 1);
		if (lcfg.readNumEntry("Version", 0) >= gversion)
			return;

 	        QDir dir(m_path);
    	        QStringList entries = dir.entryList( QDir::Files );
                QStringList dirEntries = dir.entryList( QDir::Dirs | QDir::NoSymLinks );
                dirEntries.remove( "." );
                dirEntries.remove( ".." );

                QDir globalDir( dirtree_dir );
                Q_ASSERT( globalDir.isReadable() );
                // Only copy the entries that don't exist yet in the local dir
                QStringList globalDirEntries = globalDir.entryList();
                QStringList::ConstIterator eIt = globalDirEntries.begin();
                QStringList::ConstIterator eEnd = globalDirEntries.end();
                for (; eIt != eEnd; ++eIt )
                {
                	//kdDebug(1201) << "KonqSidebarTree::scanDir dirtree_dir contains " << *eIt << endl;
                	if ( *eIt != "." && *eIt != ".." &&
				!entries.contains( *eIt ) &&
				!dirEntries.contains( *eIt ) )
			{ // we don't have that one yet -> copy it.
				QString cp("cp -R ");
				cp += KProcess::quote(dirtree_dir + *eIt);
				cp += " ";
				cp += KProcess::quote(m_path);
				kdDebug() << "SidebarWidget::intialCopy executing " << cp << endl;
				::system( QFile::encodeName(cp) );
			}
		}
		lcfg.writeEntry("Version",gversion);
		lcfg.sync();
	}
}

void Sidebar_Widget::buttonPopupActivate(int id)
{
	switch (id)
	{
		case 1:
		{
			KIconDialog kicd(this);
//			kicd.setStrictIconSize(true);
			QString iconname=kicd.selectIcon(KIcon::Small);
			kdDebug()<<"New Icon Name:"<<iconname<<endl;;
			if (!iconname.isEmpty())
			{
				KSimpleConfig ksc(m_path+m_currentButton->file);
				ksc.setGroup("Desktop Entry");
				ksc.writeEntry("Icon",iconname);
				ksc.sync();
			        QTimer::singleShot(0,this,SLOT(updateButtons()));
			}
			break;
		}
		case 2:
		{
			bool ok;
			QString newurl = KInputDialog::getText(QString::null,
				i18n("Enter a URL:"), m_currentButton->URL,&ok,this);
			if (ok)
			{
				KSimpleConfig ksc(m_path+m_currentButton->file);
				ksc.setGroup("Desktop Entry");
				ksc.writeEntry("Name",newurl);
				ksc.writeEntry("URL",newurl);
				ksc.sync();
				QTimer::singleShot(0,this,SLOT(updateButtons()));
			}
			break;
		}
		case 3:
		{
			if (KMessageBox::questionYesNo(this,i18n("<qt>Do you really want to remove the <b>%1</b> tab?</qt>").
				arg(m_currentButton->displayName))==KMessageBox::Yes)
			{
				QFile f(m_path+m_currentButton->file);
				if (!f.remove())
					qDebug("Error, file not deleted");
				QTimer::singleShot(0,this,SLOT(updateButtons()));
			}
			break;
		}
	}
}

void Sidebar_Widget::activatedMenu(int id)
{
	switch (id)
	{
		case 1:
		{
			m_singleWidgetMode = !m_singleWidgetMode;
			if ((m_singleWidgetMode) && (m_visibleViews.count()>1))
			{
				for (uint i=0; i<m_buttons.count(); i++) {
					ButtonInfo *button = m_buttons.at(i);
					if ((int) i != m_latestViewed)
					{
						if (button->dock && button->dock->isVisibleTo(this))
							showHidePage(i);
					} else {
						if (button->dock)
						{
							m_area->setMainDockWidget(button->dock);
							m_mainDockWidget->undock();
						}
					}
				}
			} else {
				if (!m_singleWidgetMode)
				{
					int tmpLatestViewed=m_latestViewed;
					m_area->setMainDockWidget(m_mainDockWidget);
	        			m_mainDockWidget->setDockSite(KDockWidget::DockTop);
				        m_mainDockWidget->setEnableDocking(KDockWidget::DockNone);
					m_mainDockWidget->show();
					if ((tmpLatestViewed>=0) && (tmpLatestViewed < (int) m_buttons.count()))
					{
						ButtonInfo *button = m_buttons.at(tmpLatestViewed);
						if (button && button->dock)
						{
							m_noUpdate=true;
							button->dock->undock();
			                                button->dock->setEnableDocking(KDockWidget::DockTop|
                                				KDockWidget::DockBottom/*|KDockWidget::DockDesktop*/);
							kdDebug()<<"Reconfiguring multi view mode"<<endl;
							m_buttonBar->setTab(tmpLatestViewed,true);
							showHidePage(tmpLatestViewed);
						}
					}
				}
			}
			break;
		}
		case 2:
		{
			m_showTabsLeft = ! m_showTabsLeft;
			doLayout();
			break;
		}
		case 3:
		{
			m_showExtraButtons = ! m_showExtraButtons;
			if(m_showExtraButtons)
			{
				m_buttonBar->button(-1)->show();
			}
			else
			{
				KMessageBox::information(this,
				i18n("You have hidden the navigation panel configuration button. To make it visible again, click the right mouse button on any of the navigation panel buttons and select \"Show Configuration Button\"."));

				m_buttonBar->button(-1)->hide();
			}
			break;
		}
		default:
			return;
	}
	m_configTimer.start(400, true);
}

void Sidebar_Widget::readConfig()
{
	m_singleWidgetMode = m_config->readBoolEntry("SingleWidgetMode",true);
	m_showExtraButtons = m_config->readBoolEntry("ShowExtraButtons",true);
	m_showTabsLeft = m_config->readBoolEntry("ShowTabsLeft", true);
	if (m_initial) {
		m_openViews = m_config->readListEntry("OpenViews");
		m_savedWidth = m_config->readNumEntry("SavedWidth",200);
		m_initial=false;
	}
}

void Sidebar_Widget::stdAction(const char *handlestd)
{
	ButtonInfo* mod = m_activeModule;

	if (!mod)
		return;
	if (!(mod->module))
		return;

	kdDebug() << "Try calling >active< module's action" << handlestd << endl;

	int id = mod->module->metaObject()->findSlot( handlestd );
  	if ( id == -1 )
		return;
	kdDebug() << "Action slot was found, it will be called now" << endl;
  	QUObject o[ 1 ];
	mod->module->qt_invoke( id, o );
  	return;
}


void Sidebar_Widget::updateButtons()
{
	//PARSE ALL DESKTOP FILES
	m_openViews = m_visibleViews;
	
	if (m_buttons.count() > 0)
	{
		for (uint i = 0; i < m_buttons.count(); i++)
		{
			ButtonInfo *button = m_buttons.at(i);
			if (button->dock)
			{
				m_noUpdate = true;
				if (button->dock->isVisibleTo(this)) {
					showHidePage(i);
				}

				delete button->module;
				delete button->dock;
			}
			m_buttonBar->removeTab(i);

		}
	}
	m_buttons.clear();

	readConfig();
	doLayout();
	createButtons();
}	

void Sidebar_Widget::createButtons()
{
	if (!m_path.isEmpty())
	{
		kdDebug()<<"m_path: "<<m_path<<endl;
		QDir dir(m_path);
		QStringList list=dir.entryList("*.desktop");
		for (QStringList::Iterator it=list.begin(); it!=list.end(); ++it)
		{
			addButton(*it);
		}
	}

	if (!m_buttonBar->button(-1)) {
		m_buttonBar->appendButton(SmallIcon("configure"), -1, m_menu,
					i18n("Configure Sidebar"));
	}

	if (m_showExtraButtons) {
		m_buttonBar->button(-1)->show();
	} else {
		m_buttonBar->button(-1)->hide();
	}

	for (uint i = 0; i < m_buttons.count(); i++)
	{
		ButtonInfo *button = m_buttons.at(i);
		if (m_openViews.contains(button->file))
		{
			m_buttonBar->setTab(i,true);
			m_noUpdate = true;
			showHidePage(i);
			if (m_singleWidgetMode) {
				break;
			}
		}
	}

	collapseExpandSidebar();
        m_noUpdate=false;
}

bool Sidebar_Widget::openURL(const class KURL &url)
{
	m_storedUrl=url;
	m_hasStoredUrl=true;
        bool ret = false;
	for (unsigned int i=0;i<m_buttons.count();i++)
	{
		ButtonInfo *button = m_buttons.at(i);
		if (button->dock)
		{
			if ((button->dock->isVisibleTo(this)) && (button->module))
			{
				ret = true;
				button->module->openURL(url);
			}
		}
	}
	return ret;
}

bool Sidebar_Widget::addButton(const QString &desktoppath,int pos)
{
	int lastbtn = m_buttons.count();
	m_buttons.resize(m_buttons.size()+1);

  	KSimpleConfig *confFile;

	kdDebug() << "addButton:" << (m_path+desktoppath) << endl;

	confFile = new KSimpleConfig(m_path+desktoppath,true);
	confFile->setGroup("Desktop Entry");

    	QString icon = confFile->readEntry("Icon","");
	QString name = confFile->readEntry("Name","");
	QString comment = confFile->readEntry("Comment","");
	QString url = confFile->readEntry("URL","");
	QString lib = confFile->readEntry("X-KDE-KonqSidebarModule","");

        delete confFile;

	if (pos == -1)
	{
	  	m_buttonBar->appendTab(SmallIcon(icon), lastbtn, name);
		ButtonInfo *bi = new ButtonInfo(desktoppath, 0, url, lib, name,
						icon, this);
		/*int id=*/m_buttons.insert(lastbtn, bi);
		KMultiTabBarTab *tab = m_buttonBar->tab(lastbtn);
		tab->installEventFilter(this);
		connect(tab,SIGNAL(clicked(int)),this,SLOT(showHidePage(int)));

		// Set Whats This help
		// This uses the comments in the .desktop files
		QWhatsThis::add(tab, comment);
	}

	return true;
}



bool Sidebar_Widget::eventFilter(QObject *obj, QEvent *ev)
{

	if (ev->type()==QEvent::MouseButtonPress && ((QMouseEvent *)ev)->button()==QMouseEvent::RightButton)
	{
		KMultiTabBarTab *bt=dynamic_cast<KMultiTabBarTab*>(obj);
		if (bt)
		{
			kdDebug()<<"Request for popup"<<endl;
			m_currentButton = 0;
			for (uint i=0;i<m_buttons.count();i++)
			{
				if (bt==m_buttonBar->tab(i))
				{
					m_currentButton = m_buttons.at(i);
					break;
				}
			}

			if (m_currentButton)
			{
				if (!m_buttonPopup)
				{
					m_buttonPopup=new KPopupMenu(this, "Sidebar_Widget::ButtonPopup");
					m_buttonPopup->insertTitle(SmallIcon("unknown"), "", 50);
					m_buttonPopup->insertItem(SmallIconSet("www"), i18n("Set URL"),2);
					m_buttonPopup->insertItem(SmallIconSet("image"), i18n("Set Icon"),1);
					m_buttonPopup->insertSeparator();
					m_buttonPopup->insertItem(SmallIconSet("remove"), i18n("Remove"),3);
					m_buttonPopup->insertSeparator();
					m_buttonPopup->insertItem(SmallIconSet("configure"), i18n("Configure Navigation Panel"), m_menu, 4);
					connect(m_buttonPopup, SIGNAL(activated(int)),
						this, SLOT(buttonPopupActivate(int)));
				}
				m_buttonPopup->setItemEnabled(2,!m_currentButton->URL.isEmpty());
			        m_buttonPopup->changeTitle(50,SmallIcon(m_currentButton->iconName),
						m_currentButton->displayName);
				m_buttonPopup->exec(QCursor::pos());
			}
			return true;

		}
	}
	return false;
}

void Sidebar_Widget::mousePressEvent(QMouseEvent *ev)
{
	if (ev->type()==QEvent::MouseButtonPress && ((QMouseEvent *)ev)->button()==QMouseEvent::RightButton)
		m_menu->exec(QCursor::pos());
}

KonqSidebarPlugin *Sidebar_Widget::loadModule(QWidget *par,QString &desktopName,QString lib_name,ButtonInfo* bi)
{
	KLibLoader *loader = KLibLoader::self();

	// try to load the library
      	KLibrary *lib = loader->library(QFile::encodeName(lib_name));
	if (lib)
	{
		// get the create_ function
		QString factory("create_%1");
		void *create = lib->symbol(QFile::encodeName(factory.arg(lib_name)));

		if (create)
		{
			// create the module

			KonqSidebarPlugin* (*func)(KInstance*,QObject *, QWidget*, QString&, const char *);
			func = (KonqSidebarPlugin* (*)(KInstance*,QObject *, QWidget *, QString&, const char *)) create;
			QString fullPath(m_path+desktopName);
			return  (KonqSidebarPlugin*)func(getInstance(),bi,par,fullPath,0);
		}
	} else {
		kdWarning() << "Module " << lib_name << " doesn't specify a library!" << endl;
	}
	return 0;
}

KParts::BrowserExtension *Sidebar_Widget::getExtension()
{
	return KParts::BrowserExtension::childObject(m_partParent);
}

bool Sidebar_Widget::createView( ButtonInfo *data)
{
	bool ret = true;
	KSimpleConfig *confFile;
	confFile = new KSimpleConfig(data->file,true);
	confFile->setGroup("Desktop Entry");

	data->dock = m_area->createDockWidget(confFile->readEntry("Name",i18n("Unknown")),0);
	data->module = loadModule(data->dock,data->file,data->libName,data);

	if (data->module == 0)
	{
		delete data->dock;
		data->dock = 0;
		ret = false;
	} else {
		data->dock->setWidget(data->module->getWidget());
		data->dock->setEnableDocking(KDockWidget::DockTop|
		KDockWidget::DockBottom/*|KDockWidget::DockDesktop*/);
		data->dock->setDockSite(KDockWidget::DockTop|KDockWidget::DockBottom);
		connectModule(data->module);
		connect(this, SIGNAL(fileSelection(const KFileItemList&)),
			data->module, SLOT(openPreview(const KFileItemList&)));

		connect(this, SIGNAL(fileMouseOver(const KFileItem&)),
			data->module, SLOT(openPreviewOnMouseOver(const KFileItem&)));
	}

	delete confFile;
	return ret;
}

void Sidebar_Widget::showHidePage(int page)
{
	ButtonInfo *info = m_buttons.at(page);
	if (!info->dock)
	{
		if (m_buttonBar->isTabRaised(page))
		{
			//SingleWidgetMode
			if (m_singleWidgetMode)
			{
				if (m_latestViewed != -1)
				{
					m_noUpdate = true;
					showHidePage(m_latestViewed);
				}
			}

			if (!createView(info))
			{
				m_buttonBar->setTab(page,false);
				return;
			}

			m_buttonBar->setTab(page,true);

			connect(info->module,
				SIGNAL(setIcon(const QString&)),
				m_buttonBar->tab(page),
				SLOT(setIcon(const QString&)));

			connect(info->module,
				SIGNAL(setCaption(const QString&)),
				m_buttonBar->tab(page),
				SLOT(setText(const QString&)));

			if (m_singleWidgetMode)
			{
				m_area->setMainDockWidget(info->dock);
				m_mainDockWidget->undock();
			} else {
				info->dock->manualDock(m_mainDockWidget,KDockWidget::DockTop,100);
			}

			info->dock->show();

			if (m_hasStoredUrl)
				info->module->openURL(m_storedUrl);
			m_visibleViews<<info->file;
			m_latestViewed=page;
		}
	} else {
		if ((!info->dock->isVisible()) && (m_buttonBar->isTabRaised(page))) {
			//SingleWidgetMode
			if (m_singleWidgetMode) {
				if (m_latestViewed != -1) {
					m_noUpdate = true;
					showHidePage(m_latestViewed);
				}
			}

			if (m_singleWidgetMode) {
				m_area->setMainDockWidget(info->dock);
				m_mainDockWidget->undock();
			} else {
				info->dock->manualDock(m_mainDockWidget,KDockWidget::DockTop,100);
			}

			info->dock->show();
			m_latestViewed = page;
			if (m_hasStoredUrl)
				info->module->openURL(m_storedUrl);
			m_visibleViews << info->file;
			m_buttonBar->setTab(page,true);
		} else {
			m_buttonBar->setTab(page,false);
			if (m_singleWidgetMode) {
				m_area->setMainDockWidget(m_mainDockWidget);
				m_mainDockWidget->show();
			}
			info->dock->undock();
			m_latestViewed = -1;
			m_visibleViews.remove(info->file);
		}
	}

	if (!m_noUpdate)
		collapseExpandSidebar();
	m_noUpdate = false;
}

void Sidebar_Widget::collapseExpandSidebar()
{
	if (m_visibleViews.count()==0)
	{
		m_somethingVisible = false;
		parentWidget()->setMaximumWidth(minimumSizeHint().width());
		QEvent *e = new QEvent( QEvent::LayoutHint );
		QApplication::postEvent( this, e );
	} else {
		m_somethingVisible = true;
		parentWidget()->setMaximumWidth(32767);
		QEvent *e = new QEvent( QEvent::LayoutHint );
		QApplication::postEvent( this, e );
	}
}

QSize Sidebar_Widget::sizeHint() const
{
        if (m_somethingVisible)
           return QSize(m_savedWidth,200);
        return minimumSizeHint();
}

void Sidebar_Widget::dockWidgetHasUndocked(KDockWidget* wid)
{
	kdDebug()<<" Sidebar_Widget::dockWidgetHasUndocked(KDockWidget*)"<<endl;
	for (unsigned int i=0;i<m_buttons.count();i++)
	{
		ButtonInfo *button = m_buttons.at(i);
		if (button->dock==wid)
		{
			if (m_buttonBar->isTabRaised(i))
			{
				m_buttonBar->setTab(i,false);
				showHidePage(i);
			}
		}
	}
}

KInstance  *Sidebar_Widget::getInstance()
{
	return ((KonqSidebar*)m_partParent)->getInstance();
}

void Sidebar_Widget::openURLRequest( const KURL &url, const KParts::URLArgs &args)
{
	getExtension()->openURLRequest(url,args);
}

void Sidebar_Widget::createNewWindow( const KURL &url, const KParts::URLArgs &args)
{
	getExtension()->createNewWindow(url,args);
}

void Sidebar_Widget::createNewWindow( const KURL &url, const KParts::URLArgs &args,
	const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part )
{
	getExtension()->createNewWindow(url,args,windowArgs,part);
}

void Sidebar_Widget::enableAction( const char * name, bool enabled )
{
 	if (sender()->parent()->isA("ButtonInfo"))
	{
		ButtonInfo *btninfo = static_cast<ButtonInfo*>(sender()->parent());
		if (btninfo)
		{
			QString n(name);
			if (n == "copy")
				btninfo->copy = enabled;
			else if (n == "cut")
				btninfo->cut = enabled;
			else if (n == "paste")
				btninfo->paste = enabled;
			else if (n == "trash")
				btninfo->trash = enabled;
			else if (n == "del")
				btninfo->del = enabled;
			else if (n == "shred")
				btninfo->shred = enabled;
			else if (n == "rename")
				btninfo->rename = enabled;
		}
	}
}


bool  Sidebar_Widget::doEnableActions()
{
 	if (!(sender()->parent()->isA("ButtonInfo")))
	{
		kdDebug()<<"Couldn't set active module, aborting"<<endl;
		return false;
	} else {
		m_activeModule=static_cast<ButtonInfo*>(sender()->parent());
		getExtension()->enableAction( "copy", m_activeModule->copy );
		getExtension()->enableAction( "cut", m_activeModule->cut );
		getExtension()->enableAction( "paste", m_activeModule->paste );
		getExtension()->enableAction( "trash", m_activeModule->trash );
		getExtension()->enableAction( "del", m_activeModule->del );
		getExtension()->enableAction( "shred", m_activeModule->shred );
		getExtension()->enableAction( "rename", m_activeModule->rename );
		return true;
	}

}

void Sidebar_Widget::popupMenu( const QPoint &global, const KFileItemList &items )
{
	if (doEnableActions())
		getExtension()->popupMenu(global,items);
}


void Sidebar_Widget::popupMenu( KXMLGUIClient *client, const QPoint &global, const KFileItemList &items )
{
	if (doEnableActions())
		getExtension()->popupMenu(client,global,items);
}

void Sidebar_Widget::popupMenu( const QPoint &global, const KURL &url,
	const QString &mimeType, mode_t mode)
{
	if (doEnableActions())
		getExtension()->popupMenu(global,url,mimeType,mode);
}

void Sidebar_Widget::popupMenu( KXMLGUIClient *client,
	const QPoint &global, const KURL &url,
	const QString &mimeType, mode_t mode )
{
	if (doEnableActions())
		getExtension()->popupMenu(client,global,url,mimeType,mode);
}

void Sidebar_Widget::connectModule(QObject *mod)
{
	if (mod->metaObject()->findSignal("started(KIO::Job*)") != -1) {
		connect(mod,SIGNAL(started(KIO::Job *)),this, SIGNAL(started(KIO::Job*)));
	}

	if (mod->metaObject()->findSignal("completed()") != -1) {
		connect(mod,SIGNAL(completed()),this,SIGNAL(completed()));
	}

	if (mod->metaObject()->findSignal("popupMenu(const QPoint&,const KURL&,const QString&,mode_t)") != -1) {
		connect(mod,SIGNAL(popupMenu( const QPoint &, const KURL &,
			const QString &, mode_t)),this,SLOT(popupMenu( const
			QPoint &, const KURL&, const QString &, mode_t)));
	}

	if (mod->metaObject()->findSignal("popupMenu(KXMLGUIClient*,const QPoint&,const KURL&,const QString&,mode_t)") != -1) {
		connect(mod,SIGNAL(popupMenu( KXMLGUIClient *, const QPoint &,
			const KURL &,const QString &, mode_t)),this,
			SLOT(popupMenu( KXMLGUIClient *, const QPoint &,
			const KURL &,const QString &, mode_t)));
	}

	if (mod->metaObject()->findSignal("popupMenu(const QPoint&,const KFileItemList&)") != -1) {
		connect(mod,SIGNAL(popupMenu( const QPoint &, const KFileItemList & )),
			this,SLOT(popupMenu( const QPoint &, const KFileItemList & )));
	}

	if (mod->metaObject()->findSignal("openURLRequest(const KURL&,const KParts::URLArgs&)") != -1) {
		connect(mod,SIGNAL(openURLRequest( const KURL &, const KParts::URLArgs &)),
			this,SLOT(openURLRequest( const KURL &, const KParts::URLArgs &)));
	}

	if (mod->metaObject()->findSignal("enableAction(const char*,bool)") != -1) {
		connect(mod,SIGNAL(enableAction( const char *, bool)),
			this,SLOT(enableAction(const char *, bool)));
	}

	if (mod->metaObject()->findSignal("createNewWindow(const KURL&,const KParts::URLArgs&)") != -1) {
		connect(mod,SIGNAL(createNewWindow( const KURL &, const KParts::URLArgs &)),
			this,SLOT(createNewWindow( const KURL &, const KParts::URLArgs &)));
	}
}



Sidebar_Widget::~Sidebar_Widget()
{
	if (m_configTimer.isActive())
		saveConfig();
	delete m_config;
	m_noUpdate = true;
	for (uint i=0;i<m_buttons.count();i++)
	{
		ButtonInfo *button = m_buttons.at(i);
		if (button->dock)
			button->dock->undock();
	}
}

void Sidebar_Widget::customEvent(QCustomEvent* ev)
{
	if (KonqFileSelectionEvent::test(ev))
	{
		emit fileSelection(static_cast<KonqFileSelectionEvent*>(ev)->selection());
	} else if (KonqFileMouseOverEvent::test(ev)) {
		if (!(static_cast<KonqFileMouseOverEvent*>(ev)->item())) {
			emit fileMouseOver(KFileItem(KURL(),QString::null,KFileItem::Unknown));
		} else {
			emit fileMouseOver(*static_cast<KonqFileMouseOverEvent*>(ev)->item());
		}
	}
}

void Sidebar_Widget::resizeEvent(QResizeEvent* ev)
{
	if (m_somethingVisible)
	{
		int newWidth = width();
		if (m_savedWidth != newWidth)
		{
			m_savedWidth = newWidth;
			m_configTimer.start(400, true);
		}
	}
	QWidget::resizeEvent(ev);
}

QSplitter *Sidebar_Widget::splitter() const
{
	QObject *p = parent();
	if (!p) return 0;
	p = p->parent();
	return static_cast<QSplitter*>(p);
}

