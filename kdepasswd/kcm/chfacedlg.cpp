/**
 *  Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>                   
 *  Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>       
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *
 *
 *  Please see the README
 *
 */

/**
 * @file UserInfo's Dialog for changing your face.
 * @author Braden MacDonald
 */

#include <qstring.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qpushbutton.h>
#include <qdir.h>
#include <qcheckbox.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <kdialogbase.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kiconview.h>
#include <kimagefilepreview.h>
#include <kimageio.h>
#include <kmessagebox.h>
#include <konq_operations.h>
#include <kurl.h>

#include "chfacedlg.h"
#include "settings.h" // KConfigXT



/**
 * TODO: It would be nice if the widget were in a .ui
 */
ChFaceDlg::ChFaceDlg(const QString& picsdir, QWidget *parent, const char *name, bool modal)
  : KDialogBase( parent, name, modal, i18n("Change your Face"), Ok|Cancel, Ok, true )
{
  QWidget *page = new QWidget(this);
  setMainWidget( page );

  QVBoxLayout *top = new QVBoxLayout(page, 0, spacingHint());

  QLabel *header = new QLabel( i18n("Select a new face:"), page );
  top->addWidget( header );

  m_FacesWidget = new KIconView( page );
  m_FacesWidget->setSelectionMode( Q3IconView::Single );
  m_FacesWidget->setItemsMovable( false );
  m_FacesWidget->setMinimumSize( 400, 200 );

  connect( m_FacesWidget, SIGNAL( selectionChanged( Q3IconViewItem * ) ), SLOT( slotFaceWidgetSelectionChanged( Q3IconViewItem * ) ) );

  connect( m_FacesWidget, SIGNAL( doubleClicked( Q3IconViewItem *, const QPoint & ) ), SLOT( slotOk() ) );

  top->addWidget( m_FacesWidget );

  // Buttons to get more pics
  QHBoxLayout * morePics = new QHBoxLayout( 0, 0, spacingHint() );
  QPushButton *browseBtn = new QPushButton( i18n("Custom &Image..."), page );
  connect( browseBtn, SIGNAL( clicked() ), SLOT( slotGetCustomImage() ) );
  morePics->addWidget( browseBtn );
#if 0
  QPushButton *acquireBtn = new QPushButton( i18n("&Acquire Image..."), page );
  acquireBtn->setEnabled( false );
  morePics->addWidget( acquireBtn );
#endif
  morePics->addStretch();
  top->addLayout( morePics );

  // Filling the icon view
  QDir facesDir( picsdir );
  if ( facesDir.exists() )
  {
    QStringList picslist = facesDir.entryList( QDir::Files );
    for ( QStringList::Iterator it = picslist.begin(); it != picslist.end(); ++it )
      new Q3IconViewItem( m_FacesWidget, (*it).section(".",0,0), QPixmap( picsdir + *it ) );
  }
  facesDir.setPath( KCFGUserAccount::userFaceDir() );
  if ( facesDir.exists() )
  {
    QStringList picslist = facesDir.entryList( QDir::Files );
    for ( QStringList::Iterator it = picslist.begin(); it != picslist.end(); ++it )
      new Q3IconViewItem( m_FacesWidget, "/"+(*it) == KCFGUserAccount::customFaceFile() ? 
		      i18n("(Custom)") : (*it).section(".",0,0),
                      QPixmap( KCFGUserAccount::userFaceDir() + *it ) );
  }

  m_FacesWidget->setResizeMode( Q3IconView::Adjust );
  //m_FacesWidget->setGridX( FACE_PIX_SIZE - 10 );
  m_FacesWidget->arrangeItemsInGrid();

  enableButtonOK( false );
  //connect( this, SIGNAL( okClicked() ), SLOT( slotSaveCustomImage() ) );

  resize( 420, 400 );
}

void ChFaceDlg::addCustomPixmap( QString imPath, bool saveCopy )
{
  QImage pix( imPath );
  // TODO: save pix to TMPDIR/userinfo-tmp,
  // then scale and copy *that* to ~/.faces

  if (pix.isNull())
  {
    KMessageBox::sorry( this, i18n("There was an error loading the image.") );
    return;
  }
  if ( (pix.width() > KCFGUserAccount::faceSize())
	|| (pix.height() > KCFGUserAccount::faceSize()) )
    pix = pix.scaled( KCFGUserAccount::faceSize(), KCFGUserAccount::faceSize(), Qt::KeepAspectRatio );// Should be no bigger than certain size.

  if ( saveCopy )
  {
    // If we should save a copy:
    QDir userfaces( KCFGUserAccount::userFaceDir() );
    if ( !userfaces.exists( ) )
      userfaces.mkdir( userfaces.absolutePath() );

    pix.save( userfaces.absolutePath() + "/.userinfo-tmp" , "PNG" );
    KonqOperations::copy( this, KonqOperations::COPY, KUrl::List( KUrl( userfaces.absolutePath() + "/.userinfo-tmp" ) ), KUrl( userfaces.absPath() + "/" + QFileInfo(imPath).fileName().section(".",0,0) ) );
#if 0
  if ( !pix.save( userfaces.absolutePath() + "/" + imPath , "PNG" ) )
    KMessageBox::sorry(this, i18n("There was an error saving the image:\n%1").arg( userfaces.absolutePath() ) );
#endif
  }

  Q3IconViewItem* newface = new Q3IconViewItem( m_FacesWidget, QFileInfo(imPath).fileName().section(".",0,0) ,QPixmap( pix) );
  newface->setKey( KCFGUserAccount::customKey() );// Add custom items to end
  m_FacesWidget->ensureItemVisible( newface );
  m_FacesWidget->setCurrentItem( newface );
}

void ChFaceDlg::slotGetCustomImage(  )
{
  QCheckBox* checkWidget = new QCheckBox( i18n("&Save copy in custom faces folder for future use"), 0 );

  KFileDialog *dlg = new KFileDialog( QDir::homePath(), KImageIO::pattern( KImageIO::Reading ),
                  this, checkWidget);

  dlg->setOperationMode( KFileDialog::Opening );
  dlg->setCaption( i18n("Choose Image") );
  dlg->setMode( KFile::File | KFile::LocalOnly );

  KImageFilePreview *ip = new KImageFilePreview( dlg );
  dlg->setPreviewWidget( ip );
  if (dlg->exec() == QDialog::Accepted)
      addCustomPixmap( dlg->selectedFile(), checkWidget->isChecked() );
  // Because we give it a parent we have to close it ourselves.
  dlg->close(true);
}

#if 0
void ChFaceDlg::slotSaveCustomImage()
{
  if ( m_FacesWidget->currentItem()->key() ==  USER_CUSTOM_KEY)
  {
    QDir userfaces( QDir::homePath() + USER_FACES_DIR );
    if ( !userfaces.exists( ) )
      userfaces.mkdir( userfaces.absolutePath() );

    if ( !m_FacesWidget->currentItem()->pixmap()->save( userfaces.absolutePath() + USER_CUSTOM_FILE , "PNG" ) )
      KMessageBox::sorry(this, i18n("There was an error saving the image:\n%1").arg( userfaces.absolutePath() ) );
  }
}
#endif

#include "chfacedlg.moc"
