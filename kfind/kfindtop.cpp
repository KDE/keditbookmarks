/***********************************************************************
 *
 *  KfindTop.cpp
 *
 ***********************************************************************/

#include <stdio.h>

#include <qsize.h>
#include <qpopupmenu.h>
#include <qlayout.h>
#include <qpixmap.h>

#include <kapp.h>
#include <kiconloader.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <ktmainwindow.h>

#include "kfoptions.h"
#include "kftabdlg.h"
#include "kfind.h"
#include "kfindtop.h"
#include <klocale.h>
#include <kaccel.h>
#include <kkeydialog.h>

#include "version.h"
#include <kglobal.h>

KfindTop::KfindTop(const char *searchPath) : KTMainWindow()
  {
//     setCaption(QString("KFind ")+KFIND_VERSION);
	_accel = new KAccel(this);

    _toolBar = new KToolBar( this, "_toolBar" );
    _toolBar->setBarPos( KToolBar::Top );
    _toolBar->show();
    enableToolBar( KToolBar::Show, addToolBar( _toolBar ) );

    _kfind = new Kfind(this,"dialog",searchPath);
    setView( _kfind, FALSE );
    _kfind->show();

    menuInit();
    toolBarInit();

    setMenu(_mainMenu);
    _mainMenu->show();

    _statusBar = new KStatusBar( this, "_statusBar");
    _statusBar->insertItem("0 file(s) found", 0);
    _statusBar->enable(KStatusBar::Hide);
    setStatusBar(_statusBar);
    enableStatusBar(false);

    connect(_kfind,SIGNAL(haveResults(bool)),
            this,SLOT(enableSaveResults(bool)));
    connect(_kfind,SIGNAL(resultSelected(bool)),
	    this,SLOT(enableMenuItems(bool)));
    connect(this,SIGNAL(deleteFile()),
 	    _kfind,SIGNAL(deleteFile()));
    connect(this,SIGNAL(properties()),
 	    _kfind,SIGNAL(properties()));
    connect(this,SIGNAL(openFolder()),
 	    _kfind,SIGNAL(openFolder()));
    connect(this,SIGNAL(saveResults()),
 	    _kfind,SIGNAL(saveResults()));
    connect(this,SIGNAL(addToArchive()),
 	    _kfind,SIGNAL(addToArchive()));
    connect(this,SIGNAL(open()),
 	    _kfind,SIGNAL(open()));
    connect(_kfind ,SIGNAL(statusChanged(const char *)),
	    this,SLOT(statusChanged(const char *)));
    connect(_kfind ,SIGNAL(enableSearchButton(bool)),
	    this,SLOT(enableSearchButton(bool)));
    connect(_kfind ,SIGNAL(enableStatusBar(bool)),
            this,SLOT(enableStatusBar(bool)));
  }

KfindTop::~KfindTop()
  {
    delete _fileMenu;
    delete _editMenu;
    delete _optionMenu;
    delete _helpMenu;
    delete _kfind;
    delete _mainMenu;
    delete _toolBar;
    delete _statusBar;
  };

void KfindTop::menuInit()
{
  _fileMenu   = new QPopupMenu;
  _editMenu   = new QPopupMenu;
  _optionMenu = new QPopupMenu;
  _helpMenu   = new QPopupMenu;

  _accel->connectItem(KAccel::Find, _kfind, SLOT(startSearch()));
  _accel->connectItem(KAccel::Open, this,   SLOT(open()));
  _accel->connectItem(KAccel::Save, this,   SLOT(saveResults()));
  _accel->connectItem(KAccel::Undo, this,   SLOT(undo()));
  _accel->connectItem(KAccel::Copy, this,   SLOT(copySelection()));
  _accel->connectItem(KAccel::Cut,  this,   SLOT(cut()));
  _accel->connectItem(KAccel::Quit, kapp,   SLOT(quit()));
  _accel->insertItem(i18n("Stop Search"), "search", Key_Escape);
  _accel->insertItem(i18n("Delete"),      "delete", Key_Delete);

  _accel->readSettings();

  fileStart = _fileMenu->insertItem(i18n("&Start search"), _kfind,
				    SLOT(startSearch()));
  _accel->changeMenuAccel(_fileMenu, fileStart, KAccel::Find);
  fileStop = _fileMenu->insertItem(i18n("S&top search"), _kfind,
				   SLOT(stopSearch()));
  _accel->changeMenuAccel(_fileMenu, fileStop, "search");
  _fileMenu->setItemEnabled(fileStop, FALSE);
  _fileMenu->insertSeparator();

  openWithM = _fileMenu->insertItem(i18n("&Open"), this, SIGNAL(open()));
  _accel->changeMenuAccel(_fileMenu, openWithM, KAccel::Open);
  toArchM = _fileMenu->insertItem(i18n("&Add to archive"),
				  this,SIGNAL(addToArchive()));
  _fileMenu->insertSeparator();

  deleteM = _fileMenu->insertItem(i18n("&Delete"),
				  this,SIGNAL(deleteFile()));
  _accel->changeMenuAccel(_fileMenu, deleteM, "delete");
  propsM = _fileMenu->insertItem(i18n("&Properties"),
				 this,SIGNAL(properties()));
  _fileMenu->insertSeparator();

  openFldrM = _fileMenu->insertItem(i18n("Open Containing &Folder"),
				    this,SIGNAL(openFolder()));
  _fileMenu->insertSeparator();

  saveSearchM = _fileMenu->insertItem(i18n("&Save Search"),
				      this, SIGNAL(saveResults()));
  _accel->changeMenuAccel(_fileMenu, saveSearchM, KAccel::Save);
  _fileMenu->insertSeparator();

  quitM = _fileMenu->insertItem(i18n("&Quit"), kapp, SLOT(quit()));
  _accel->changeMenuAccel(_fileMenu, quitM, KAccel::Quit);
  for(int i=openWithM;i>quitM;i--)
    _fileMenu->setItemEnabled(i,FALSE);

  int undo = _editMenu->insertItem(i18n("&Undo"),
				   this, SIGNAL(undo()));
  _accel->changeMenuAccel(_editMenu, undo, KAccel::Undo);
  _editMenu->insertSeparator();
  int cut = _editMenu->insertItem(i18n("&Cut"),
				  this, SIGNAL(cut()));
  _accel->changeMenuAccel(_editMenu, cut, KAccel::Cut);
  editCopy =  _editMenu->insertItem(i18n("&Copy"),
				    this, SLOT(copySelection()));
  _accel->changeMenuAccel(_editMenu, editCopy, KAccel::Copy);
  _editMenu->insertSeparator();
  editSelectAll = _editMenu->insertItem(i18n("&Select All"),
					this,SIGNAL(selectAll()) );
  editUnselectAll = _editMenu->insertItem(i18n("Unse&lect All"),
					  this,SIGNAL(unselectAll()) );

  _editMenu->setItemEnabled( undo      , FALSE );
  _editMenu->setItemEnabled( cut       , FALSE );
  _editMenu->setItemEnabled( editCopy  , FALSE );
  _editMenu->setItemEnabled( editSelectAll, FALSE );
  _editMenu->setItemEnabled( editUnselectAll, FALSE );

  _optionMenu->insertItem(i18n("Configure &Key Bindings..."),
			  this, SLOT(keyBindings()));
  _optionMenu->insertItem(i18n("&Preferences ..."),
			  this,SLOT(prefs()));

  QString tmp = i18n("KFind %1\nFrontend to find utility\nMiroslav Fl�dr <flidr@kky.zcu.cz>\n\nSpecial thanks to Stephan Kulow\n<coolo@kde.org>")
    .arg(KFIND_VERSION);
  _helpMenu=helpMenu( tmp );

  _mainMenu = new KMenuBar(this, "_mainMenu");
  _mainMenu->insertItem( i18n("&File"), _fileMenu);
  _mainMenu->insertItem( i18n("&Edit"), _editMenu);
  _mainMenu->insertItem( i18n("&Options"), _optionMenu);
  _mainMenu->insertSeparator();
  _mainMenu->insertItem( i18n("&Help"), _helpMenu );
}

void KfindTop::toolBarInit()
{
  QPixmap icon;

  icon = BarIcon("search");
  _toolBar->insertButton( icon, 0, SIGNAL(clicked()),
			  _kfind, SLOT(startSearch()),
			  TRUE, i18n("Start Search"));

  icon = BarIcon("reload");
  _toolBar->insertButton( icon, 1, SIGNAL(clicked()),
			  _kfind, SLOT(newSearch()),
			  TRUE, i18n("New Search"));

  icon = BarIcon("stop");
  _toolBar->insertButton( icon, 2, SIGNAL(clicked()),
			  _kfind, SLOT(stopSearch()),
			  FALSE, i18n("Stop Search"));

  _toolBar->insertSeparator();

  icon = BarIcon("openfile");
  _toolBar->insertButton( icon, 3,SIGNAL(clicked()),
			  _kfind,SIGNAL(open()),
			  FALSE, i18n("Open"));

  icon = BarIcon("archive");
  _toolBar->insertButton( icon, 4,SIGNAL(clicked()),
			  _kfind,SIGNAL(addToArchive()),
			  FALSE, i18n("Add to archive"));

  icon = BarIcon("delete");
  _toolBar->insertButton( icon, 5,SIGNAL(clicked()),
			  _kfind,SIGNAL(deleteFile()),
			  FALSE, i18n("Delete"));

  icon = BarIcon("info");
  _toolBar->insertButton( icon, 6,SIGNAL(clicked()),
			  _kfind,SIGNAL(properties()),
			  FALSE, i18n("Properties"));

  icon = BarIcon("fileopen");
  _toolBar->insertButton( icon, 7,SIGNAL(clicked()),
			  _kfind,SIGNAL(openFolder()),
			  FALSE, i18n("Open Containing Folder"));

  icon = BarIcon("save");
  _toolBar->insertButton( icon, 8,SIGNAL(clicked()),
			  _kfind,SIGNAL(saveResults()),
			  FALSE, i18n("Save Search Results"));

  _toolBar->insertSeparator();

  icon = BarIcon("contents");
  _toolBar->insertButton( icon, 9, SIGNAL( clicked() ),
			  kapp, SLOT( appHelpActivated() ),
			  TRUE, i18n("Help"));

  icon = BarIcon("exit");
  _toolBar->insertButton( icon, 10, SIGNAL( clicked() ),
                          KApplication::kApplication(), SLOT( quit() ),
			  TRUE, i18n("Quit"));
}

void KfindTop::enableSaveResults(bool enable)
{
  _toolBar->setItemEnabled(8, enable);
  _fileMenu->setItemEnabled(saveSearchM, enable);
  _editMenu->setItemEnabled(editSelectAll, enable);
  _editMenu->setItemEnabled(editUnselectAll, enable);
}

void KfindTop::enableMenuItems(bool enable)
{
  int i;
  for(i=openWithM;i>quitM+1;i--)
    _fileMenu->setItemEnabled(i,enable);
  for(i=3;i<8;i++)
    _toolBar->setItemEnabled(i,enable);

  _editMenu->setItemEnabled( editCopy, TRUE );
}

void KfindTop::enableSearchButton(bool enable)
{
  _fileMenu->setItemEnabled(fileStart, enable);
  _fileMenu->setItemEnabled(fileStop, !enable);

  _toolBar->setItemEnabled(0,enable);
  _toolBar->setItemEnabled(2,!enable);
}

void KfindTop::enableStatusBar(bool enable)
{
  if (enable)
    KTMainWindow::enableStatusBar(KStatusBar::Show);
  else
    KTMainWindow::enableStatusBar(KStatusBar::Hide);
}

void KfindTop::statusChanged(const char *str)
{
  _statusBar->changeItem((char *)str, 0);
}

void KfindTop::keyBindings()
{
    KKeyDialog::configureKeys(_accel);
}

void KfindTop::prefs()
{
  KfOptions *prefs = new KfOptions(this,0L);
  prefs->show();
}

void KfindTop::copySelection()
{
  _kfind->copySelection();
}
