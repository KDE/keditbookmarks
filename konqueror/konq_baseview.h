#ifndef __konq_baseview_h__
#define __konq_baseview_h__

#include "konqueror.h"

#include <opPart.h>
#include <qstring.h>

/**
 * The base class for all views in konqueror
 */
class KonqBaseView : virtual public OPPartIf,
		     virtual public Konqueror::View_skel
{
public:
  KonqBaseView();
  ~KonqBaseView();
  
  virtual void init();
  virtual void cleanUp();
  
  virtual bool event( const char *event, const CORBA::Any &value );
  virtual bool mappingFillMenuView( Konqueror::View::EventFillMenu viewMenu );
  virtual bool mappingFillMenuEdit( Konqueror::View::EventFillMenu viewMenu );
  virtual bool mappingCreateViewToolBar( Konqueror::View::EventCreateViewToolBar viewToolBar );
  virtual bool mappingOpenURL( Konqueror::EventOpenURL eventURL );

  virtual char *url();

  virtual Konqueror::View::HistoryEntry *saveState();
  virtual void restoreState( const Konqueror::View::HistoryEntry &history );

protected:
  QString m_strURL;
  CORBA::String_var debug_ViewName;
};

#endif
