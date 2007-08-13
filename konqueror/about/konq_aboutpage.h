#ifndef __konq_aboutpage_h__
#define __konq_aboutpage_h__

#include <kparts/factory.h>
#include <khtml_part.h>

class KHTMLPart;
class KComponentData;

class KonqAboutPageFactory : public KParts::Factory
{
public:
    KonqAboutPageFactory( QObject *parent = 0 );
    virtual ~KonqAboutPageFactory();

    virtual KParts::Part *createPartObject( QWidget *parentWidget, QObject *parent,
                                            const char *classname, const QStringList &args );

    static const KComponentData &componentData() { return *s_instance; }

    static QString launch();
    static QString intro();
    static QString specs();
    static QString tips();
    static QString plugins();

private:
    static QString loadFile( const QString& file );

    static KComponentData *s_instance;
    static QString *s_launch_html, *s_intro_html, *s_specs_html, *s_tips_html, *s_plugins_html;
};

class KonqAboutPage : public KHTMLPart
{
    Q_OBJECT
public:
    KonqAboutPage( QWidget *parentWidget, QObject *parent );
    ~KonqAboutPage();

    virtual bool openUrl( const KUrl &url );

    virtual bool openFile();

    virtual void saveState( QDataStream &stream );
    virtual void restoreState( QDataStream &stream );

protected:
    virtual bool urlSelected( const QString &url, int button, int state, const QString &target,
                              const KParts::OpenUrlArguments& args = KParts::OpenUrlArguments(),
                              const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments() );

private:
    void serve( const QString&, const QString& );

    KHTMLPart *m_doc;
    //KonqMainWindow *m_mainWindow;
    QString m_htmlDoc;
    QString m_what;
};

#endif
