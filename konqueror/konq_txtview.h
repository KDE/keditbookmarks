#ifndef __konq_txtview_h__
#define __konq_txtview_h__

#include "konq_baseview.h"

#include <qmultilineedit.h>

class KonqTxtView : public QMultiLineEdit,
                    public KonqBaseView,
		    virtual public Konqueror::TxtView_skel
{
  Q_OBJECT
  
public:
  KonqTxtView();
  virtual ~KonqTxtView();
  
  virtual bool mappingOpenURL( Konqueror::EventOpenURL eventURL );
  virtual bool mappingFillMenuView( Konqueror::View::EventFillMenu viewMenu );
  virtual bool mappingFillMenuEdit( Konqueror::View::EventFillMenu editMenu );

  virtual void stop();
  virtual char *viewName() { return CORBA::string_dup( "KonquerorTxtView" ); }

  virtual Konqueror::View::HistoryEntry *saveState();
  virtual void restoreState( const Konqueror::View::HistoryEntry &history );

protected slots:
  void slotFinished( int );
  void slotRedirection( int, const char * );
  void slotData( int, const char *, int );
  void slotError( int, int, const char * );

protected:
  virtual void mousePressEvent( QMouseEvent *e );  
  virtual void dragEnterEvent( QDragEnterEvent *e );
  virtual void dropEvent( QDropEvent *e );
    
private:
  int m_jobId;
};

#endif
