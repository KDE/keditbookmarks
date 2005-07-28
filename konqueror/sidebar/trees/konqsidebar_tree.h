#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <qlabel.h>
#include <qlayout.h>
#include <kparts/part.h>
#include <kparts/factory.h>
#include <kparts/browserextension.h>
#include <kdialogbase.h>
#include <qcombobox.h>
#include <qstringlist.h>
#include <klocale.h>
#include <qlineedit.h>
class KonqSidebarTree;

class KonqSidebar_Tree: public KonqSidebarPlugin
        {
                Q_OBJECT
                public:
                KonqSidebar_Tree(KInstance *instance,QObject *parent,QWidget *widgetParent, QString &desktopName_, const char* name=0);
                ~KonqSidebar_Tree();
                virtual void *provides(const QString &);
//		void emitStatusBarText (const QString &);
                virtual QWidget *getWidget();
                protected:
                        class QWidget *widget;
                        class KonqSidebarTree *tree;
                        virtual void handleURL(const KURL &url);
		protected slots:
			void copy();
			void cut();
			void paste();
			void trash();
			void del();
			void shred();
			void rename();
signals:
			void openURLRequest( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
  			void createNewWindow( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
			void popupMenu( const QPoint &global, const KURL &url,
					const QString &mimeType, mode_t mode = (mode_t)-1 );
			void popupMenu( const QPoint &global, const KFileItemList &items );
			void enableAction( const char * name, bool enabled );
        };

#endif
