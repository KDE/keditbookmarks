#include <kapplication.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <qlabel.h>

#include <kparts/browserinterface.h>
#include <kparts/browserextension.h>

#include "nspluginloader.h"

#include "plugin_part.h"
#include "plugin_part.moc"


class PluginBrowserExtension : public KParts::BrowserExtension
{
    friend class PluginPart;
public:
    PluginBrowserExtension( KParts::ReadOnlyPart *parent,
                          const char *name = 0L )
        : KParts::BrowserExtension( parent, name ) {}
    ~PluginBrowserExtension() {}

    // ATTENTION: you -CANNOT- add data members here
};


PluginLiveConnectExtension::PluginLiveConnectExtension(PluginPart* part) 
: KParts::LiveConnectExtension(part), _part(part) {
}

PluginLiveConnectExtension::~PluginLiveConnectExtension() {
}

bool PluginLiveConnectExtension::get(const unsigned long, const QString &field, Type &type, unsigned long &retobj, QString &value) {
Q_UNUSED(type);
Q_UNUSED(retobj);
Q_UNUSED(value);
    kdDebug(1432) << "PLUGIN:LiveConnect::get " << field << endl;
    return false;
}

bool PluginLiveConnectExtension::call(const unsigned long, const QString &func, const QStringList &args, Type &type, unsigned long &retobjid, QString &value) {
Q_UNUSED(type);
Q_UNUSED(retobjid);
Q_UNUSED(value);
    kdDebug(1432) << "PLUGIN:LiveConnect::call " << func << " args: " << args << endl;
      return false;
}

bool PluginLiveConnectExtension::put( const unsigned long, const QString &field, const QString &value) {
    kdDebug(1432) << "PLUGIN:LiveConnect::put " << field << " " << value << endl;
    if (field == "__nsplugin") {
        __nsplugin = value;
        return true;
    } else if (field.lower() == "src") {
        _part->changeSrc(value);
        return true;
    }
    return false;
}

QString PluginLiveConnectExtension::evalJavaScript( const QString & script )
{
    kdDebug(1432) << "PLUGIN:LiveConnect::evalJavaScript " << script << endl;
    ArgList args;
    QString jscode;
    jscode.sprintf("this.__nsplugin=eval(\"%s\")", script.latin1());
    args.push_back(qMakePair(KParts::LiveConnectExtension::TypeString, jscode));
    emit partEvent(0, "eval", args);
    return __nsplugin;
}

extern "C"
{
  /**
   * This function is the 'main' function of this part.  It takes
   * the form 'void *init_lib<library name>()  It always returns a
   * new factory object
   */
  void *init_libnsplugin() {
      return new PluginFactory;
  }
}


NSPluginCallback::NSPluginCallback(PluginPart *part)
  : DCOPObject()
{
    _part = part;
}


void NSPluginCallback::requestURL(QString url, QString target)
{
    _part->requestURL( url.latin1(), target.latin1() );
}


void NSPluginCallback::statusMessage( QString msg )
{
    _part->statusMessage( msg );
}


QString NSPluginCallback::evalJavaScript( QString script )
{
    return _part->evalJavaScript( script );
}
/**
 * We need one static instance of the factory for our C 'main'
 * function
 */
KInstance *PluginFactory::s_instance = 0L;


PluginFactory::PluginFactory()
{
    kdDebug(1432) << "PluginFactory::PluginFactory" << endl;
    s_instance = 0;

    // preload plugin loader
    _loader = NSPluginLoader::instance();
}


PluginFactory::~PluginFactory()
{
   kdDebug(1432) << "PluginFactory::~PluginFactory" << endl;

   _loader->release();

   if ( s_instance ) {
       delete s_instance->aboutData();
       delete s_instance;
   }
   s_instance = 0;
}

KParts::Part * PluginFactory::createPartObject(QWidget *parentWidget, const char *widgetName,
                                         QObject *parent, const char *name,
                                         const char */*classname*/, const QStringList &args)
{
    kdDebug(1432) << "PluginFactory::create" << endl;
    KParts::Part *obj = new PluginPart(parentWidget, widgetName, parent, name, args);
    return obj;
}


KInstance *PluginFactory::instance()
{
    kdDebug(1432) << "PluginFactory::instance" << endl;

    if ( !s_instance )
        s_instance = new KInstance( aboutData() );
    return s_instance;
}

KAboutData *PluginFactory::aboutData()
{
    KAboutData *about = new KAboutData("plugin", I18N_NOOP("plugin"), "1.99");
    return about;
}


/**************************************************************************/


PluginPart::PluginPart(QWidget *parentWidget, const char *widgetName, QObject *parent,
                       const char *name, const QStringList &args)
    : KParts::ReadOnlyPart(parent, name), _widget(0), _args(args)
{
    setInstance(PluginFactory::instance());
    kdDebug(1432) << "PluginPart::PluginPart" << endl;

    // we have to keep the class name of KParts::PluginBrowserExtension
    // to let khtml find it
    _extension = static_cast<PluginBrowserExtension*>(new KParts::BrowserExtension(this));
    _liveconnect = new PluginLiveConnectExtension(this);

    // create
    _loader = NSPluginLoader::instance();
    _callback = new NSPluginCallback(this);

    // create a canvas to insert our widget
    _canvas = new PluginCanvasWidget( parentWidget, widgetName );
    _canvas->setFocusPolicy( QWidget::ClickFocus );
    _canvas->setBackgroundMode( QWidget::NoBackground );
    setWidget(_canvas);
    _canvas->show();
    QObject::connect( _canvas, SIGNAL(resized(int,int)),
                      this, SLOT(pluginResized(int,int)) );
}


PluginPart::~PluginPart()
{
    kdDebug(1432) << "PluginPart::~PluginPart" << endl;

    delete _callback;
    _loader->release();
}


bool PluginPart::openURL(const KURL &url)
{
    kdDebug(1432) << "-> PluginPart::openURL" << endl;

    m_url = url;
    QString surl = url.url();
    QString smime = _extension->urlArgs().serviceType;
    bool embed = false;

    // handle arguments
    QStringList argn, argv;

    QStringList::Iterator it = _args.begin();
    for ( ; it != _args.end(); ) {

        int equalPos = (*it).find("=");
        if (equalPos>0) {

            QString name = (*it).left(equalPos).upper();
            QString value = (*it).mid(equalPos+1);
            if (value[0] == '"' && value[value.length()-1] == '"')
                value = value.mid(1, value.length()-2);

            kdDebug(1432) << "name=" << name << " value=" << value << endl;

            if (!name.isEmpty()) {
                // hack to pass view mode from khtml
                if ( name=="__KHTML__PLUGINEMBED" ) {
                    embed = true;
                    kdDebug(1432) << "__KHTML__PLUGINEMBED found" << endl;
                } else {
                    argn << name;
                    argv << value;
                }
            }
        }

        it++;
    }

    if (surl.isEmpty()) {
        kdDebug(1432) << "<- PluginPart::openURL - false (no url passed to nsplugin)" << endl;
        return false;
    }

    // status messages
    emit setWindowCaption( url.prettyURL() );
    emit setStatusBarText( i18n("Loading Netscape plugin for %1").arg(url.prettyURL()) );

    // create plugin widget
    NSPluginInstance *inst = _loader->newInstance( _canvas, surl, smime, embed,
                                                   argn, argv,
                                                   kapp->dcopClient()->appId(),
                                                   _callback->objId());

    if ( inst ) {
        _widget = inst;
    } else {
        QLabel *label = new QLabel( i18n("Unable to load Netscape plugin for %1").arg(url.url()), _canvas );
        label->setAlignment( AlignCenter | WordBreak );
        _widget = label;
    }

    _widget->resize( _canvas->width(), _canvas->height() );
    _widget->show();

    kdDebug(1432) << "<- PluginPart::openURL = " << (inst!=0) << endl;
    return inst!=0;
}


bool PluginPart::closeURL()
{
    kdDebug(1432) << "PluginPart::closeURL" << endl;
    delete _widget;
    _widget = 0;
    return true;
}


void PluginPart::reloadPage()
{
    kdDebug(1432) << "PluginPart::reloadPage()" << endl;
    _extension->browserInterface()->callMethod("goHistory(int)", 0);
}

void PluginPart::requestURL(const QString& url, const QString& target)
{
    kdDebug(1432) << "PluginPart::requestURL( url=" << url
                  << ", target=" << target << endl;

    if (url.startsWith("javascript:history.back")) {
        _extension->browserInterface()->callMethod("goHistory(int)", -1);
        return;
    }

    KURL new_url(this->url(), url);
    KParts::URLArgs args;
    args.frameName = target;

    emit _extension->openURLRequest( new_url, args );
}

QString PluginPart::evalJavaScript( const QString & script )
{
    return _liveconnect->evalJavaScript( script );
}

void PluginPart::statusMessage( QString msg )
{
    kdDebug(1422) << "PluginPart::statusMessage " << msg << endl;
    emit setStatusBarText( msg );
}


void PluginPart::pluginResized(int w, int h)
{
    kdDebug(1432) << "PluginPart::pluginResized()" << endl;

    if ( _widget )
        _widget->resize( w, h );
}


void PluginPart::changeSrc(const QString& url) {
    closeURL();
    openURL(url);
}

void PluginCanvasWidget::resizeEvent(QResizeEvent *ev)
{
    QWidget::resizeEvent(ev);
    emit resized(width(), height());
}


