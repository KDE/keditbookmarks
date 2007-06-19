#ifndef __konq_browseriface_h__
#define __konq_browseriface_h__

#include <kparts/browserinterface.h>

class KonqView;

class KonqBrowserInterface : public KParts::BrowserInterface
{
    Q_OBJECT
    Q_PROPERTY( uint historyLength READ historyLength )
public:
    explicit KonqBrowserInterface( KonqView *view );

    uint historyLength() const;

public Q_SLOTS:
    void goHistory( int );

private:
    KonqView *m_view;
};

#endif
