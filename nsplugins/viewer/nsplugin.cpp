/*

  This is an encapsulation of the  Netscape plugin API.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>
                2003-2005 George Staikos <staikos@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#include "NSPluginCallbackIface_stub.h"


#include <stdlib.h>
#include <unistd.h>

#include <qdict.h>
#include <qdir.h>
#include <qfile.h>
#include <qtimer.h>

#include "kxt.h"
#include "nsplugin.h"
#include "resolve.h"

#ifdef Bool
#undef Bool
#endif

#include <dcopclient.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kio/netaccess.h>
#include <klibloader.h>
#include <klocale.h>
#include <kprocess.h>
#include <kprotocolmanager.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <kurl.h>

#include <X11/Intrinsic.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>

// provide these symbols when compiling with gcc 3.x

#if defined __GNUC__ && defined __GNUC_MINOR__
# define KDE_GNUC_PREREQ(maj, min) \
  ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define KDE_GNUC_PREREQ(maj, min) 0
#endif


#if defined(__GNUC__) && KDE_GNUC_PREREQ(3,0)
extern "C" void* __builtin_new(size_t s)
{
   return operator new(s);
}

extern "C" void __builtin_delete(void* p)
{
   operator delete(p);
}

extern "C" void* __builtin_vec_new(size_t s)
{
   return operator new[](s);
}

extern "C" void __builtin_vec_delete(void* p)
{
   operator delete[](p);
}

extern "C" void __pure_virtual()
{
   abort();
}
#endif

// server side functions -----------------------------------------------------

// allocate memory
void *g_NPN_MemAlloc(uint32 size)
{
   void *mem = ::malloc(size);

   //kdDebug(1431) << "g_NPN_MemAlloc(), size=" << size << " allocated at " << mem << endl;

   return mem;
}


// free memory
void g_NPN_MemFree(void *ptr)
{
   //kdDebug(1431) << "g_NPN_MemFree() at " << ptr << endl;
   if (ptr)
     ::free(ptr);
}

uint32 g_NPN_MemFlush(uint32 size)
{
   Q_UNUSED(size);
   //kdDebug(1431) << "g_NPN_MemFlush()" << endl;
   // MAC OS only..  we don't use this
   return 0;
}


// redraw
void g_NPN_ForceRedraw(NPP /*instance*/)
{
   // http://devedge.netscape.com/library/manuals/2002/plugin/1.0/npn_api3.html#999401
   // FIXME
   kdDebug(1431) << "g_NPN_ForceRedraw() [unimplemented]" << endl;
}


// invalidate rect
void g_NPN_InvalidateRect(NPP /*instance*/, NPRect* /*invalidRect*/)
{
   // http://devedge.netscape.com/library/manuals/2002/plugin/1.0/npn_api7.html#999503
   // FIXME
   kdDebug(1431) << "g_NPN_InvalidateRect() [unimplemented]" << endl;
}


// invalidate region
void g_NPN_InvalidateRegion(NPP /*instance*/, NPRegion /*invalidRegion*/)
{
   // http://devedge.netscape.com/library/manuals/2002/plugin/1.0/npn_api8.html#999528
   // FIXME
   kdDebug(1431) << "g_NPN_InvalidateRegion() [unimplemented]" << endl;
}


// get value
NPError g_NPN_GetValue(NPP /*instance*/, NPNVariable variable, void *value)
{
   kdDebug(1431) << "g_NPN_GetValue(), variable=" << static_cast<int>(variable) << endl;

   switch (variable)
   {
      case NPNVxDisplay:
         *(void**)value = qt_xdisplay();
         return NPERR_NO_ERROR;
      case NPNVxtAppContext:
         *(void**)value = XtDisplayToApplicationContext(qt_xdisplay());
         return NPERR_NO_ERROR;
      case NPNVjavascriptEnabledBool:
         *(bool*)value = true;
         return NPERR_NO_ERROR;
      case NPNVasdEnabledBool:
         // SmartUpdate - we don't do this
         *(bool*)value = false;
         return NPERR_NO_ERROR;
      case NPNVisOfflineBool:
         // Offline browsing - no thanks
         *(bool*)value = false;
         return NPERR_NO_ERROR;
      default:
         return NPERR_INVALID_PARAM;
   }
}


NPError g_NPN_DestroyStream(NPP instance, NPStream* stream,
                          NPReason reason)
{
   // FIXME: is this correct?  I imagine it is not.  (GS)
   kdDebug(1431) << "g_NPN_DestroyStream()" << endl;

   NSPluginInstance *inst = (NSPluginInstance*) instance->ndata;
   inst->streamFinished( (NSPluginStream *)stream->ndata );

   switch (reason) {
   case NPRES_DONE:
      return NPERR_NO_ERROR;
   case NPRES_USER_BREAK:
      // FIXME: notify the user
   case NPRES_NETWORK_ERR:
      // FIXME: notify the user
   default:
      return NPERR_GENERIC_ERROR;
   }
}


NPError g_NPN_RequestRead(NPStream* /*stream*/, NPByteRange* /*rangeList*/)
{
   // http://devedge.netscape.com/library/manuals/2002/plugin/1.0/npn_api16.html#999734
   kdDebug(1431) << "g_NPN_RequestRead() [unimplemented]" << endl;

   // FIXME
   return NPERR_GENERIC_ERROR;
}

NPError g_NPN_NewStream(NPP /*instance*/, NPMIMEType /*type*/,
                      const char* /*target*/, NPStream** /*stream*/)
{
   // http://devedge.netscape.com/library/manuals/2002/plugin/1.0/npn_api12.html#999628
   kdDebug(1431) << "g_NPN_NewStream() [unimplemented]" << endl;

   // FIXME
   // This creates a stream from the plugin to the browser of type "type" to
   // display in "target"
   return NPERR_GENERIC_ERROR;
}

int32 g_NPN_Write(NPP /*instance*/, NPStream* /*stream*/, int32 /*len*/, void* /*buf*/)
{
   // http://devedge.netscape.com/library/manuals/2002/plugin/1.0/npn_api21.html#999859
   kdDebug(1431) << "g_NPN_Write() [unimplemented]" << endl;

   // FIXME
   return 0;
}


// URL functions
NPError g_NPN_GetURL(NPP instance, const char *url, const char *target)
{
   kdDebug(1431) << "g_NPN_GetURL: url=" << url << " target=" << target << endl;

   NSPluginInstance *inst = static_cast<NSPluginInstance*>(instance->ndata);
   if (inst) {
      inst->requestURL( QString::fromLatin1(url), QString::null,
                        QString::fromLatin1(target), 0 );
   }

   return NPERR_NO_ERROR;
}


NPError g_NPN_GetURLNotify(NPP instance, const char *url, const char *target,
                         void* notifyData)
{
    kdDebug(1431) << "g_NPN_GetURLNotify: url=" << url << " target=" << target << " inst=" << (void*)instance << endl;
   NSPluginInstance *inst = static_cast<NSPluginInstance*>(instance->ndata);
   if (inst) {
      kdDebug(1431) << "g_NPN_GetURLNotify: ndata=" << (void*)inst << endl;
      inst->requestURL( QString::fromLatin1(url), QString::null,
                        QString::fromLatin1(target), notifyData, true );
   }

   return NPERR_NO_ERROR;
}


NPError g_NPN_PostURLNotify(NPP instance, const char* url, const char* target,
                     uint32 len, const char* buf, NPBool file, void* notifyData)
{
// http://devedge.netscape.com/library/manuals/2002/plugin/1.0/npn_api14.html
   kdDebug(1431) << "g_NPN_PostURLNotify() [incomplete]" << endl;
   kdDebug(1431) << "url=[" << url << "] target=[" << target << "]" << endl;
   QByteArray postdata;
   KParts::URLArgs args;

   if (len == 0) {
      return NPERR_NO_DATA;
   }

   if (file) { // buf is a filename
      QFile f(buf);
      if (!f.open(IO_ReadOnly)) {
         return NPERR_FILE_NOT_FOUND;
      }

      // FIXME: this will not work because we need to strip the header out!
      postdata = f.readAll();
      f.close();
   } else {    // buf is raw data
      // First strip out the header
      const char *previousStart = buf;
      uint32 l;
      bool previousCR = true;

      for (l = 1;; l++) {
         if (l == len) {
            break;
         }

         if (buf[l-1] == '\n' || (previousCR && buf[l-1] == '\r')) {
            if (previousCR) { // header is done!
               if ((buf[l-1] == '\r' && buf[l] == '\n') ||
                   (buf[l-1] == '\n' &&  buf[l] == '\r'))
                  l++;
               l++;
               previousStart = &buf[l-1];
               break;
            }

            QString thisLine = QString::fromLatin1(previousStart, &buf[l-1] - previousStart).stripWhiteSpace();

            previousStart = &buf[l];
            previousCR = true;

            kdDebug(1431) << "Found header line: [" << thisLine << "]" << endl;
            if (thisLine.startsWith("Content-Type: ")) {
               args.setContentType(thisLine);
            }
         } else {
            previousCR = false;
         }
      }

      postdata.duplicate(previousStart, len - l + 1);
   }

   kdDebug(1431) << "Post data: " << postdata.size() << " bytes" << endl;
#if 0
   QFile f("/tmp/nspostdata");
   f.open(IO_WriteOnly);
   f.writeBlock(postdata);
   f.close();
#endif

   if (!target || !*target) {
      // Send the results of the post to the plugin
      // (works by default)
   } else if (!strcmp(target, "_current") || !strcmp(target, "_self") ||
              !strcmp(target, "_top")) {
      // Unload the plugin, put the results in the frame/window that the
      // plugin was loaded in
      // FIXME
   } else if (!strcmp(target, "_new") || !strcmp(target, "_blank")){
      // Open a new browser window and write the results there
      // FIXME
   } else {
      // Write the results to the specified frame
      // FIXME
   }

   NSPluginInstance *inst = static_cast<NSPluginInstance*>(instance->ndata);
   if (inst && !inst->normalizedURL(QString::fromLatin1(url)).isNull()) {
      inst->postURL( QString::fromLatin1(url), postdata, args.contentType(),
                     QString::fromLatin1(target), notifyData, args, true );
   } else {
      // Unsupported / insecure
      return NPERR_INVALID_URL;
   }

   return NPERR_NO_ERROR;
}


NPError g_NPN_PostURL(NPP instance, const char* url, const char* target,
                    uint32 len, const char* buf, NPBool file)
{
// http://devedge.netscape.com/library/manuals/2002/plugin/1.0/npn_api13.html
   kdDebug(1431) << "g_NPN_PostURL()" << endl;
   kdDebug(1431) << "url=[" << url << "] target=[" << target << "]" << endl;
   QByteArray postdata;
   KParts::URLArgs args;

   if (len == 0) {
      return NPERR_NO_DATA;
   }

   if (file) { // buf is a filename
      QFile f(buf);
      if (!f.open(IO_ReadOnly)) {
         return NPERR_FILE_NOT_FOUND;
      }

      // FIXME: this will not work because we need to strip the header out!
      postdata = f.readAll();
      f.close();
   } else {    // buf is raw data
      // First strip out the header
      const char *previousStart = buf;
      uint32 l;
      bool previousCR = true;

      for (l = 1;; l++) {
         if (l == len) {
            break;
         }

         if (buf[l-1] == '\n' || (previousCR && buf[l-1] == '\r')) {
            if (previousCR) { // header is done!
               if ((buf[l-1] == '\r' && buf[l] == '\n') ||
                   (buf[l-1] == '\n' &&  buf[l] == '\r'))
                  l++;
               l++;
               previousStart = &buf[l-1];
               break;
            }

            QString thisLine = QString::fromLatin1(previousStart, &buf[l-1] - previousStart).stripWhiteSpace();

            previousStart = &buf[l];
            previousCR = true;

            kdDebug(1431) << "Found header line: [" << thisLine << "]" << endl;
            if (thisLine.startsWith("Content-Type: ")) {
               args.setContentType(thisLine);
            }
         } else {
            previousCR = false;
         }
      }

      postdata.duplicate(previousStart, len - l + 1);
   }

   kdDebug(1431) << "Post data: " << postdata.size() << " bytes" << endl;
#if 0
   QFile f("/tmp/nspostdata");
   f.open(IO_WriteOnly);
   f.writeBlock(postdata);
   f.close();
#endif

   if (!target || !*target) {
      // Send the results of the post to the plugin
      // (works by default)
   } else if (!strcmp(target, "_current") || !strcmp(target, "_self") ||
              !strcmp(target, "_top")) {
      // Unload the plugin, put the results in the frame/window that the
      // plugin was loaded in
      // FIXME
   } else if (!strcmp(target, "_new") || !strcmp(target, "_blank")){
      // Open a new browser window and write the results there
      // FIXME
   } else {
      // Write the results to the specified frame
      // FIXME
   }

   NSPluginInstance *inst = static_cast<NSPluginInstance*>(instance->ndata);
   if (inst && !inst->normalizedURL(QString::fromLatin1(url)).isNull()) {
      inst->postURL( QString::fromLatin1(url), postdata, args.contentType(),
                     QString::fromLatin1(target), 0L, args, false );
   } else {
      // Unsupported / insecure
      return NPERR_INVALID_URL;
   }

   return NPERR_NO_ERROR;
}


// display status message
void g_NPN_Status(NPP instance, const char *message)
{
   kdDebug(1431) << "g_NPN_Status(): " << message << endl;

   if (!instance)
      return;

   // turn into an instance signal
   NSPluginInstance *inst = (NSPluginInstance*) instance->ndata;

   inst->emitStatus(message);
}


// inquire user agent
const char *g_NPN_UserAgent(NPP /*instance*/)
{
    KProtocolManager kpm;
    QString agent = kpm.userAgentForHost("nspluginviewer");
    kdDebug(1431) << "g_NPN_UserAgent() = " << agent << endl;
    return agent.latin1();
}


// inquire version information
void g_NPN_Version(int *plugin_major, int *plugin_minor, int *browser_major, int *browser_minor)
{
   kdDebug(1431) << "g_NPN_Version()" << endl;

   // FIXME: Use the sensible values
   *browser_major = NP_VERSION_MAJOR;
   *browser_minor = NP_VERSION_MINOR;

   *plugin_major = NP_VERSION_MAJOR;
   *plugin_minor = NP_VERSION_MINOR;
}


void g_NPN_ReloadPlugins(NPBool reloadPages)
{
   // http://devedge.netscape.com/library/manuals/2002/plugin/1.0/npn_api15.html#999713
   kdDebug(1431) << "g_NPN_ReloadPlugins()" << endl;
   KProcess p;
   p << KGlobal::dirs()->findExe("nspluginscan");

   if (reloadPages) {
      // This is the proper way, but it cannot be done because we have no
      // handle to the caller!  How stupid!  We cannot force all konqi windows
      // to reload - that would be evil.
      //p.start(KProcess::Block);
      // Let's only allow the caller to be reloaded, not everything.
      //if (_callback)
      //   _callback->reloadPage();
      p.start(KProcess::DontCare);
   } else {
      p.start(KProcess::DontCare);
   }
}


// JAVA functions
JRIEnv *g_NPN_GetJavaEnv()
{
   kdDebug(1431) << "g_NPN_GetJavaEnv() [unimplemented]" << endl;
   // FIXME - what do these do?  I can't find docs, and even Mozilla doesn't
   //         implement them
   return 0;
}


jref g_NPN_GetJavaPeer(NPP /*instance*/)
{
   kdDebug(1431) << "g_NPN_GetJavaPeer() [unimplemented]" << endl;
   // FIXME - what do these do?  I can't find docs, and even Mozilla doesn't
   //         implement them
   return 0;
}


NPError g_NPN_SetValue(NPP /*instance*/, NPPVariable variable, void* /*value*/)
{
   kdDebug(1431) << "g_NPN_SetValue() [unimplemented]" << endl;
   switch (variable) {
   case NPPVpluginWindowBool:
      // FIXME
      // If true, the plugin is windowless.  If false, it is in a window.
   case NPPVpluginTransparentBool:
      // FIXME
      // If true, the plugin is displayed transparent
   default:
      return NPERR_GENERIC_ERROR;
   }
}





/******************************************************************/

void
NSPluginInstance::forwarder(Widget w, XtPointer cl_data, XEvent * event, Boolean * cont)
{
  Q_UNUSED(w);
  NSPluginInstance *inst = (NSPluginInstance*)cl_data;
  *cont = True;
  if (inst->_form == 0 || event->xkey.window == XtWindow(inst->_form))
    return;
  *cont = False;
  event->xkey.window = XtWindow(inst->_form);
  event->xkey.subwindow = None;
  XtDispatchEvent(event);
}


NSPluginInstance::NSPluginInstance(NPP privateData, NPPluginFuncs *pluginFuncs,
                                   KLibrary *handle, int width, int height,
                                   QString src, QString /*mime*/,
                                   QString appId, QString callbackId,
                                   bool embed,
                                   QObject *parent, const char* name )
   : DCOPObject(), QObject( parent, name ) 
{
    Q_UNUSED(embed);
   _firstResize = true;
   _visible = false;
   _npp = privateData;
   _npp->ndata = this;
   _destroyed = false;
   _handle = handle;
   _width = width;
   _height = height;
   _tempFiles.setAutoDelete( true );
   _streams.setAutoDelete( true );
   _waitingRequests.setAutoDelete( true );
   _callback = new NSPluginCallbackIface_stub( appId.latin1(), callbackId.latin1() );

   KURL base(src);
   base.setFileName( QString::null );
   _baseURL = base.url();

   memcpy(&_pluginFuncs, pluginFuncs, sizeof(_pluginFuncs));

   _timer = new QTimer( this );
   connect( _timer, SIGNAL(timeout()), SLOT(timer()) );

   kdDebug(1431) << "NSPluginInstance::NSPluginInstance" << endl;
   kdDebug(1431) << "pdata = " << _npp->pdata << endl;
   kdDebug(1431) << "ndata = " << _npp->ndata << endl;

   if (width == 0)
      width = 1600;

   if (height == 0)
      height = 1200;

   // create drawing area
   Arg args[7];
   Cardinal nargs = 0;
   XtSetArg(args[nargs], XtNwidth, width); nargs++;
   XtSetArg(args[nargs], XtNheight, height); nargs++;
   XtSetArg(args[nargs], XtNborderWidth, 0); nargs++;

   String n, c;
   XtGetApplicationNameAndClass(qt_xdisplay(), &n, &c);

   _toplevel = XtAppCreateShell("drawingArea", c, applicationShellWidgetClass,
                                qt_xdisplay(), args, nargs);

   // What exactly does widget mapping mean? Without this call the widget isn't
   // embedded correctly. With it the viewer doesn't show anything in standalone mode.
   //if (embed)
      XtSetMappedWhenManaged(_toplevel, False);
   XtRealizeWidget(_toplevel);

   // Create form window that is searched for by flash plugin
   _form = XtVaCreateWidget("form", compositeWidgetClass, _toplevel, NULL);
   XtSetArg(args[nargs], XtNvisual, QPaintDevice::x11AppVisual()); nargs++;
   XtSetArg(args[nargs], XtNdepth, QPaintDevice::x11AppDepth()); nargs++;
   XtSetArg(args[nargs], XtNcolormap, QPaintDevice::x11AppColormap()); nargs++;
   XtSetValues(_form, args, nargs);
   XSync(qt_xdisplay(), false);

   // From mozilla - not sure if it's needed yet, nor what to use for embedder
#if 0
   /* this little trick seems to finish initializing the widget */
#if XlibSpecificationRelease >= 6
   XtRegisterDrawable(qt_xdisplay(), embedderid, _toplevel);
#else
   _XtRegisterWindow(embedderid, _toplevel);
#endif
#endif
   XtRealizeWidget(_form);
   XtManageChild(_form);

   // Register forwarder
   XtAddEventHandler(_toplevel, (KeyPressMask|KeyReleaseMask), 
                     False, forwarder, (XtPointer)this );
   XtAddEventHandler(_form, (KeyPressMask|KeyReleaseMask), 
                     False, forwarder, (XtPointer)this );
   XSync(qt_xdisplay(), false);
}

NSPluginInstance::~NSPluginInstance()
{
   kdDebug(1431) << "-> ~NSPluginInstance" << endl;
   destroy();
   kdDebug(1431) << "<- ~NSPluginInstance" << endl;
}


void NSPluginInstance::destroy()
{
    if ( !_destroyed ) {

        kdDebug(1431) << "delete streams" << endl;
        _waitingRequests.clear();

	shutdown();

        for( NSPluginStreamBase *s=_streams.first(); s!=0; ) {
            NSPluginStreamBase *next = _streams.next();
            s->stop();
            s = next;
        }

        _streams.clear();

        kdDebug(1431) << "delete callbacks" << endl;
        delete _callback;
        _callback = 0;

        kdDebug(1431) << "destroy plugin" << endl;
        NPSavedData *saved = 0;

        // As of 7/31/01, nsplugin crashes when used with Qt
        // linked with libGL if the destroy function is called.
        // A patch on that date hacked out the following call.
        // On 11/17/01, Jeremy White has reenabled this destroy
        // in a an attempt to better understand why this crash
        // occurs so that the real problem can be found and solved.
        // It's possible that a flaw in the SetWindow call
        // caused the crash and it is now fixed.
        if ( _pluginFuncs.destroy )
            _pluginFuncs.destroy( _npp, &saved );

        if (saved && saved->len && saved->buf)
          g_NPN_MemFree(saved->buf);
        if (saved)
          g_NPN_MemFree(saved);

        XtRemoveEventHandler(_form, (KeyPressMask|KeyReleaseMask), 
                             False, forwarder, (XtPointer)this);
        XtRemoveEventHandler(_toplevel, (KeyPressMask|KeyReleaseMask), 
                             False, forwarder, (XtPointer)this);
        XtDestroyWidget(_form);
	_form = 0;
        XtDestroyWidget(_toplevel);
	_toplevel = 0;

        if (_npp) {
            ::free(_npp);   // matched with malloc() in newInstance
        }

        _destroyed = true;
    }
}


void NSPluginInstance::shutdown()
{
    NSPluginClass *cls = dynamic_cast<NSPluginClass*>(parent());
    //destroy();
    if (cls) {
        cls->destroyInstance( this );
    }
}


void NSPluginInstance::timer()
{
    if (!_visible) {
         _timer->start( 100, true );
         return;
    }

    //_streams.clear();

    // start queued requests
    kdDebug(1431) << "looking for waiting requests" << endl;
    while ( _waitingRequests.head() ) {
        kdDebug(1431) << "request found" << endl;
        Request req( *_waitingRequests.head() );
        _waitingRequests.remove();

        QString url;

        // make absolute url
        if ( req.url.left(11).lower()=="javascript:" )
            url = req.url;
        else if ( KURL::isRelativeURL(req.url) ) {
            KURL bu( _baseURL );
            KURL absUrl( bu, req.url );
            url = absUrl.url();
        } else if ( req.url[0]=='/' && KURL(_baseURL).hasHost() ) {
            KURL absUrl( _baseURL );
            absUrl.setPath( req.url );
            url = absUrl.url();
        } else
            url = req.url;

        // non empty target = frame target
        if ( !req.target.isEmpty())
        {
            if (_callback)
            {
                if ( req.post ) {
                    _callback->postURL( url, req.target, req.data, req.mime );
                } else {
                    _callback->requestURL( url, req.target );
                }
                if ( req.notify ) {
                    NPURLNotify( req.url, NPRES_DONE, req.notify );
                }
            }
        } else {
            if (!url.isEmpty())
            {
                kdDebug(1431) << "Starting new stream " << req.url << endl;

                if (req.post) {
                    // create stream
                    NSPluginStream *s = new NSPluginStream( this );
                    connect( s, SIGNAL(finished(NSPluginStreamBase*)),
                             SLOT(streamFinished(NSPluginStreamBase*)) );
                    _streams.append( s );

                    kdDebug() << "posting to " << url << endl;

                    emitStatus( i18n("Submitting data to %1").arg(url) );
                    s->post( url, req.data, req.mime, req.notify, req.args );
                } else if (url.lower().startsWith("javascript:")){
                    if (_callback) {
                        static int _jsrequestid = 0;
			_jsrequests.insert(_jsrequestid, new Request(req));
                        _callback->evalJavaScript(_jsrequestid++, url.mid(11));
                    } else {
                        kdDebug() << "No callback for javascript: url!" << endl;
                    }
                } else {
                    // create stream
                    NSPluginStream *s = new NSPluginStream( this );
                    connect( s, SIGNAL(finished(NSPluginStreamBase*)),
                             SLOT(streamFinished(NSPluginStreamBase*)) );
                    _streams.append( s );

                    kdDebug() << "getting " << url << endl;

                    emitStatus( i18n("Requesting %1").arg(url) );
                    s->get( url, req.mime, req.notify, req.reload );
                }

                //break;
            }
        }
    }
}


QString NSPluginInstance::normalizedURL(const QString& url) const {
    KURL bu( _baseURL );
    KURL inURL(bu, url);
    KConfig cfg("kcmnspluginrc", true);
    cfg.setGroup("Misc");

    if (!cfg.readBoolEntry("HTTP URLs Only", false) ||
	inURL.protocol() == "http" ||
        inURL.protocol() == "https" ||
        inURL.protocol() == "javascript") {
        return inURL.url();
    }

    // Allow: javascript:, http, https, or no protocol (match loading)
    kdDebug(1431) << "NSPluginInstance::normalizedURL - I don't think so.  http or https only!" << endl;
    return QString::null;
}


void NSPluginInstance::requestURL( const QString &url, const QString &mime,
                                   const QString &target, void *notify, bool forceNotify, bool reload )
{
    // Generally this should already be done, but let's be safe for now.
    QString nurl = normalizedURL(url);
    if (nurl.isNull()) {
        return;
    }

    kdDebug(1431) << "NSPluginInstance::requestURL url=" << nurl << " target=" << target << " notify=" << notify << endl;
    _waitingRequests.enqueue( new Request( nurl, mime, target, notify, forceNotify, reload ) );
    _timer->start( 100, true );
}


void NSPluginInstance::postURL( const QString &url, const QByteArray& data,
                                const QString &mime,
                                const QString &target, void *notify,
                                const KParts::URLArgs& args, bool forceNotify )
{
    // Generally this should already be done, but let's be safe for now.
    QString nurl = normalizedURL(url);
    if (nurl.isNull()) {
        return;
    }

    kdDebug(1431) << "NSPluginInstance::postURL url=" << nurl << " target=" << target << " notify=" << notify << endl;
    _waitingRequests.enqueue( new Request( nurl, data, mime, target, notify, args, forceNotify) );
    _timer->start( 100, true );
}


void NSPluginInstance::emitStatus(const QString &message)
{
    if( _callback )
      _callback->statusMessage( message );
}


void NSPluginInstance::streamFinished( NSPluginStreamBase* strm )
{
   kdDebug(1431) << "-> NSPluginInstance::streamFinished" << endl;
   emitStatus( QString::null );
   _streams.remove(strm); // deletes automatically
   _timer->start( 100, true );
}


int NSPluginInstance::setWindow(int remove)
{
   if (remove)
   {
      NPSetWindow(0);
      return NPERR_NO_ERROR;
   }

   kdDebug(1431) << "-> NSPluginInstance::setWindow" << endl;

   _win.x = 0;
   _win.y = 0;
   _win.height = _height;
   _win.width = _width;
   _win.type = NPWindowTypeWindow;

   // Well, the docu says sometimes, this is only used on the
   // MAC, but sometimes it says it's always. Who knows...
   _win.clipRect.top = 0;
   _win.clipRect.left = 0;
   _win.clipRect.bottom = _height;
   _win.clipRect.right = _width;

   _win.window = (void*) XtWindow(_form);
   kdDebug(1431) << "Window ID = " << _win.window << endl;

   _win_info.type = NP_SETWINDOW;
   _win_info.display = XtDisplay(_form);
   _win_info.visual = DefaultVisualOfScreen(XtScreen(_form));
   _win_info.colormap = DefaultColormapOfScreen(XtScreen(_form));
   _win_info.depth = DefaultDepthOfScreen(XtScreen(_form));

   _win.ws_info = &_win_info;

   NPError error = NPSetWindow( &_win );

   kdDebug(1431) << "<- NSPluginInstance::setWindow = " << error << endl;
   return error;
}


static void resizeWidgets(Window w, int width, int height) {
   Window rroot, parent, *children;
   unsigned int nchildren = 0;

   if (XQueryTree(qt_xdisplay(), w, &rroot, &parent, &children, &nchildren)) {
      for (unsigned int i = 0; i < nchildren; i++) {
         XResizeWindow(qt_xdisplay(), children[i], width, height);
      }
      XFree(children);
   }
}


void NSPluginInstance::resizePlugin(int w, int h)
{
   if (!_visible)
      return;

   if (w == _width && h == _height)
      return;

   kdDebug(1431) << "-> NSPluginInstance::resizePlugin( w=" << w << ", h=" << h << " ) " << endl;

   _width = w;
   _height = h;

   XResizeWindow(qt_xdisplay(), XtWindow(_form), w, h);
   XResizeWindow(qt_xdisplay(), XtWindow(_toplevel), w, h);

   Arg args[7];
   Cardinal nargs = 0;
   XtSetArg(args[nargs], XtNwidth, _width); nargs++;
   XtSetArg(args[nargs], XtNheight, _height); nargs++;
   XtSetArg(args[nargs], XtNvisual, QPaintDevice::x11AppVisual()); nargs++;
   XtSetArg(args[nargs], XtNdepth, QPaintDevice::x11AppDepth()); nargs++;
   XtSetArg(args[nargs], XtNcolormap, QPaintDevice::x11AppColormap()); nargs++;
   XtSetArg(args[nargs], XtNborderWidth, 0); nargs++;

   XtSetValues(_toplevel, args, nargs);
   XtSetValues(_form, args, nargs);

   resizeWidgets(XtWindow(_form), _width, _height);

   if (_firstResize) {
      _firstResize = false;
      setWindow();
   }

   kdDebug(1431) << "<- NSPluginInstance::resizePlugin" << endl;
}


void NSPluginInstance::javascriptResult(int id, QString result) {
    QMap<int, Request*>::iterator i = _jsrequests.find( id );
    if (i != _jsrequests.end()) {
        Request *req = i.data();
        _jsrequests.remove( i );
        NSPluginStream *s = new NSPluginStream( this );
        connect( s, SIGNAL(finished(NSPluginStreamBase*)),
                 SLOT(streamFinished(NSPluginStreamBase*)) );
        _streams.append( s );

        int len = result.length();
        s->create( req->url, QString("text/plain"), req->notify, req->forceNotify );
        kdDebug(1431) << "javascriptResult has been called with: "<<result<<endl;
        if (len > 0) {
            QByteArray data(len + 1);
            memcpy(data.data(), result.latin1(), len);
            data[len] = 0;
            s->process(data, 0);
        } else {
            len = 7; //  "unknown"
            QByteArray data(len + 1);
            memcpy(data.data(), "unknown", len);
            data[len] = 0;
            s->process(data, 0);
        }
        s->finish(false);

        delete req;
    }
}


NPError NSPluginInstance::NPGetValue(NPPVariable variable, void *value)
{
    if( value==0 ) {
        kdDebug() << "FIXME: value==0 in NSPluginInstance::NPGetValue" << endl;
        return NPERR_GENERIC_ERROR;
    }

    if (!_pluginFuncs.getvalue)
        return NPERR_GENERIC_ERROR;

    NPError error = _pluginFuncs.getvalue(_npp, variable, value);

    CHECK(GetValue,error);
}


NPError NSPluginInstance::NPSetValue(NPNVariable variable, void *value)
{
    if( value==0 ) {
        kdDebug() << "FIXME: value==0 in NSPluginInstance::NPSetValue" << endl;
        return NPERR_GENERIC_ERROR;
    }

    if (!_pluginFuncs.setvalue)
        return NPERR_GENERIC_ERROR;

    NPError error = _pluginFuncs.setvalue(_npp, variable, value);

    CHECK(SetValue,error);
}


NPError NSPluginInstance::NPSetWindow(NPWindow *window)
{
    if( window==0 ) {
        kdDebug() << "FIXME: window==0 in NSPluginInstance::NPSetWindow" << endl;
        return NPERR_GENERIC_ERROR;
    }

    if (!_pluginFuncs.setwindow)
        return NPERR_GENERIC_ERROR;

    NPError error = _pluginFuncs.setwindow(_npp, window);

    CHECK(SetWindow,error);
}


NPError NSPluginInstance::NPDestroyStream(NPStream *stream, NPReason reason)
{
    if( stream==0 ) {
        kdDebug() << "FIXME: stream==0 in NSPluginInstance::NPDestroyStream" << endl;
        return NPERR_GENERIC_ERROR;
    }

    if (!_pluginFuncs.destroystream)
        return NPERR_GENERIC_ERROR;

    NPError error = _pluginFuncs.destroystream(_npp, stream, reason);

    CHECK(DestroyStream,error);
}


NPError NSPluginInstance::NPNewStream(NPMIMEType type, NPStream *stream, NPBool seekable, uint16 *stype)
{
    if( stream==0 ) {
        kdDebug() << "FIXME: stream==0 in NSPluginInstance::NPNewStream" << endl;
        return NPERR_GENERIC_ERROR;
    }

    if( stype==0 ) {
        kdDebug() << "FIXME: stype==0 in NSPluginInstance::NPNewStream" << endl;
        return NPERR_GENERIC_ERROR;
    }

    if (!_pluginFuncs.newstream)
        return NPERR_GENERIC_ERROR;

    NPError error = _pluginFuncs.newstream(_npp, type, stream, seekable, stype);

    CHECK(NewStream,error);
}


void NSPluginInstance::NPStreamAsFile(NPStream *stream, const char *fname)
{
    if( stream==0 ) {
        kdDebug() << "FIXME: stream==0 in NSPluginInstance::NPStreamAsFile" << endl;
        return;
    }

    if( fname==0 ) {
        kdDebug() << "FIXME: fname==0 in NSPluginInstance::NPStreamAsFile" << endl;
        return;
    }

    if (!_pluginFuncs.asfile)
        return;

    _pluginFuncs.asfile(_npp, stream, fname);
}


int32 NSPluginInstance::NPWrite(NPStream *stream, int32 offset, int32 len, void *buf)
{
    if( stream==0 ) {
        kdDebug() << "FIXME: stream==0 in NSPluginInstance::NPWrite" << endl;
        return 0;
    }

    if( buf==0 ) {
        kdDebug() << "FIXME: buf==0 in NSPluginInstance::NPWrite" << endl;
        return 0;
    }

    if (!_pluginFuncs.write)
        return 0;

    return _pluginFuncs.write(_npp, stream, offset, len, buf);
}


int32 NSPluginInstance::NPWriteReady(NPStream *stream)
{
    if( stream==0 ) {
        kdDebug() << "FIXME: stream==0 in NSPluginInstance::NPWriteReady" << endl;
        return 0;
    }

    if (!_pluginFuncs.writeready)
        return 0;

    return _pluginFuncs.writeready(_npp, stream);
}


void NSPluginInstance::NPURLNotify(QString url, NPReason reason, void *notifyData)
{
   if (!_pluginFuncs.urlnotify)
      return;

   _pluginFuncs.urlnotify(_npp, url.ascii(), reason, notifyData);
}


void NSPluginInstance::addTempFile(KTempFile *tmpFile)
{
   _tempFiles.append(tmpFile);
}

/*
 *   We have to call this after we reparent the widget otherwise some plugins
 *   like the ones based on WINE get very confused. (their coordinates are not
 *   adjusted for the mouse at best)
 */
void NSPluginInstance::displayPlugin()
{
   // display plugin
   setWindow();

   _visible = true;
   kdDebug(1431) << "<- NSPluginInstance::displayPlugin = " << (void*)this << endl;
}


/***************************************************************************/

NSPluginViewer::NSPluginViewer( QCString dcopId,
                                QObject *parent, const char *name )
   : DCOPObject(dcopId), QObject( parent, name ) 
{
    _classes.setAutoDelete( true );
    connect(KApplication::dcopClient(),
            SIGNAL(applicationRemoved(const QCString&)),
            this,
            SLOT(appUnregistered(const QCString&)));
}


NSPluginViewer::~NSPluginViewer()
{
   kdDebug(1431) << "NSPluginViewer::~NSPluginViewer" << endl;
}


void NSPluginViewer::appUnregistered(const QCString& id) {
   if (id.isEmpty()) {
      return;
   }

   QDictIterator<NSPluginClass> it(_classes);
   NSPluginClass *c;
   while ( (c = it.current()) ) {
      QString key = it.currentKey();
      ++it;
      if (c->app() == id) {
         _classes.remove(key);
      }
   }

   if (_classes.isEmpty()) {
      shutdown();
   }
}


void NSPluginViewer::shutdown()
{
   kdDebug(1431) << "NSPluginViewer::shutdown" << endl;
   _classes.clear();
#if QT_VERSION < 0x030100
   quitXt();
#else
   qApp->quit();
#endif
}


DCOPRef NSPluginViewer::newClass( QString plugin )
{
   kdDebug(1431) << "NSPluginViewer::NewClass( " << plugin << ")" << endl;

   // search existing class
   NSPluginClass *cls = _classes[ plugin ];
   if ( !cls ) {
       // create new class
       cls = new NSPluginClass( plugin, this );
       QCString id = "";
       DCOPClient *dc = callingDcopClient();
       if (dc) {
          id = dc->senderId();
       }
       cls->setApp(id);
       if ( cls->error() ) {
           kdError(1431) << "Can't create plugin class" << endl;
           delete cls;
           return DCOPRef();
       }

       _classes.insert( plugin, cls );
   }

   return DCOPRef( kapp->dcopClient()->appId(), cls->objId() );
}


/****************************************************************************/


NSPluginClass::NSPluginClass( const QString &library,
                              QObject *parent, const char *name )
   : DCOPObject(), QObject( parent, name ) 
{
    // initialize members
    _handle = KLibLoader::self()->library(QFile::encodeName(library));
    _libname = library;
    _constructed = false;
    _error = true;
    _instances.setAutoDelete( true );
    _NP_GetMIMEDescription = 0;
    _NP_Initialize = 0;
    _NP_Shutdown = 0;

    _timer = new QTimer( this );
    connect( _timer, SIGNAL(timeout()), SLOT(timer()) );

    // check lib handle
    if (!_handle) {
        kdDebug(1431) << "Could not dlopen " << library << endl;
        return;
    }

    // get exported lib functions
    _NP_GetMIMEDescription = (NP_GetMIMEDescriptionUPP *)_handle->symbol("NP_GetMIMEDescription");
    _NP_Initialize = (NP_InitializeUPP *)_handle->symbol("NP_Initialize");
    _NP_Shutdown = (NP_ShutdownUPP *)_handle->symbol("NP_Shutdown");

    // check for valid returned ptrs
    if (!_NP_GetMIMEDescription) {
        kdDebug(1431) << "Could not get symbol NP_GetMIMEDescription" << endl;
        return;
    }

    if (!_NP_Initialize) {
        kdDebug(1431) << "Could not get symbol NP_Initialize" << endl;
        return;
    }

    if (!_NP_Shutdown) {
        kdDebug(1431) << "Could not get symbol NP_Shutdown" << endl;
        return;
    }

    // initialize plugin
    kdDebug(1431) << "Plugin library " << library << " loaded!" << endl;
    _constructed = true;
    _error = initialize()!=NPERR_NO_ERROR;
}


NSPluginClass::~NSPluginClass()
{
    _instances.clear();
    _trash.clear();
    shutdown();
    if (_handle)
      _handle->unload();
}


void NSPluginClass::timer()
{
    // delete instances
    for ( NSPluginInstance *it=_trash.first(); it!=0; it=_trash.next() )
        _instances.remove(it);

    _trash.clear();
}


int NSPluginClass::initialize()
{
   kdDebug(1431) << "NSPluginClass::Initialize()" << endl;

   if ( !_constructed )
      return NPERR_GENERIC_ERROR;

   // initialize nescape exported functions
   memset(&_pluginFuncs, 0, sizeof(_pluginFuncs));
   memset(&_nsFuncs, 0, sizeof(_nsFuncs));

   _pluginFuncs.size = sizeof(_pluginFuncs);
   _nsFuncs.size = sizeof(_nsFuncs);
   _nsFuncs.version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
   _nsFuncs.geturl = g_NPN_GetURL;
   _nsFuncs.posturl = g_NPN_PostURL;
   _nsFuncs.requestread = g_NPN_RequestRead;
   _nsFuncs.newstream = g_NPN_NewStream;
   _nsFuncs.write = g_NPN_Write;
   _nsFuncs.destroystream = g_NPN_DestroyStream;
   _nsFuncs.status = g_NPN_Status;
   _nsFuncs.uagent = g_NPN_UserAgent;
   _nsFuncs.memalloc = g_NPN_MemAlloc;
   _nsFuncs.memfree = g_NPN_MemFree;
   _nsFuncs.memflush = g_NPN_MemFlush;
   _nsFuncs.reloadplugins = g_NPN_ReloadPlugins;
   _nsFuncs.getJavaEnv = g_NPN_GetJavaEnv;
   _nsFuncs.getJavaPeer = g_NPN_GetJavaPeer;
   _nsFuncs.geturlnotify = g_NPN_GetURLNotify;
   _nsFuncs.posturlnotify = g_NPN_PostURLNotify;
   _nsFuncs.getvalue = g_NPN_GetValue;
   _nsFuncs.setvalue = g_NPN_SetValue;
   _nsFuncs.invalidaterect = g_NPN_InvalidateRect;
   _nsFuncs.invalidateregion = g_NPN_InvalidateRegion;
   _nsFuncs.forceredraw = g_NPN_ForceRedraw;

   // initialize plugin
   NPError error = _NP_Initialize(&_nsFuncs, &_pluginFuncs);
   CHECK(Initialize,error);
}


QString NSPluginClass::getMIMEDescription()
{
   return _NP_GetMIMEDescription();
}


void NSPluginClass::shutdown()
{
    kdDebug(1431) << "NSPluginClass::shutdown error=" << _error << endl;
    if( _NP_Shutdown && !_error )
        _NP_Shutdown();
}


DCOPRef NSPluginClass::newInstance( QString url, QString mimeType, bool embed,
                                    QStringList argn, QStringList argv,
                                    QString appId, QString callbackId, bool reload )
{
   kdDebug(1431) << "-> NSPluginClass::NewInstance" << endl;

   if ( !_constructed )
       return DCOPRef();

   // copy parameters over
   unsigned int argc = argn.count();
   char **_argn = new char*[argc];
   char **_argv = new char*[argc];
   QString src = url;
   int width = 0;
   int height = 0;
   QString baseURL = url;

   for (unsigned int i=0; i<argc; i++)
   {
      const char *n = (const char*)argn[i].ascii();
      const char *v = (const char*)argv[i].ascii();

      _argn[i] = strdup(n);
      _argv[i] = strdup(v);

      if (!strcasecmp(_argn[i], "WIDTH")) width = argv[i].toInt();
      if (!strcasecmp(_argn[i], "HEIGHT")) height = argv[i].toInt();
      if (!strcasecmp(_argn[i], "__KHTML__PLUGINBASEURL")) baseURL = _argv[i];
      kdDebug(1431) << "argn=" << _argn[i] << " argv=" << _argv[i] << endl;
   }

   // create plugin instance
   char mime[256];
   strncpy(mime, mimeType.ascii(), 255);
   mime[255] = 0;
   NPP npp = (NPP)malloc(sizeof(NPP_t));   // I think we should be using
                                           // malloc here, just to be safe,
                                           // since the nsplugin plays with
                                           // this thing
   memset(npp, 0, sizeof(NPP_t));
   npp->ndata = NULL;

   // Create plugin instance object
   NSPluginInstance *inst = new NSPluginInstance( npp, &_pluginFuncs, _handle,
                                                  width, height, baseURL, mimeType,
                                                  appId, callbackId, embed, this );

   // create plugin instance
   NPError error = _pluginFuncs.newp(mime, npp, embed ? NP_EMBED : NP_FULL,
                                     argc, _argn, _argv, 0);
   kdDebug(1431) << "NPP_New = " << (int)error << endl;

   // free arrays with arguments
   delete [] _argn;
   delete [] _argv;

   // check for error
   if ( error!=NPERR_NO_ERROR)
   {
      delete inst;
      //delete npp;    double delete!
      kdDebug(1431) << "<- PluginClass::NewInstance = 0" << endl;
      return DCOPRef();
   }

   // create source stream
   if ( !src.isEmpty() )
      inst->requestURL( src, mimeType, QString::null, 0, false, reload );

   _instances.append( inst );
   return DCOPRef(kapp->dcopClient()->appId(), inst->objId());
}


void NSPluginClass::destroyInstance( NSPluginInstance* inst )
{
    // mark for destruction
    _trash.append( inst );
    timer(); //_timer->start( 0, TRUE );
}


/****************************************************************************/

NSPluginStreamBase::NSPluginStreamBase( NSPluginInstance *instance )
   : QObject( instance ), _instance(instance), _stream(0), _tempFile(0L),
     _pos(0), _queue(0), _queuePos(0), _error(false)
{
   _informed = false;
}


NSPluginStreamBase::~NSPluginStreamBase()
{
   if (_stream) {
      _instance->NPDestroyStream( _stream, NPRES_USER_BREAK );
      if (_stream && _stream->url)
          free(const_cast<char*>(_stream->url));
      delete _stream;
      _stream = 0;
   }

   delete _tempFile;
   _tempFile = 0;
   delete _queue;
   _queue = 0;
}


void NSPluginStreamBase::stop()
{
    finish( true );
}

void NSPluginStreamBase::inform()
{

    if (! _informed)
    {
        KURL src(_url);

        _informed = true;

        // inform the plugin
        _instance->NPNewStream( _mimeType.isEmpty() ? (char *) "text/plain" :  (char*)_mimeType.ascii(),
                    _stream, false, &_streamType );
        kdDebug(1431) << "NewStream stype=" << _streamType << " url=" << _url << " mime=" << _mimeType << endl;

        // prepare data transfer
        _tempFile = 0L;

        if ( _streamType==NP_ASFILE || _streamType==NP_ASFILEONLY ) {
            _onlyAsFile = _streamType==NP_ASFILEONLY;
            if ( KURL(_url).isLocalFile() )  {
                kdDebug(1431) << "local file" << endl;
                // local file can be passed directly
                _fileURL = KURL(_url).path();

                // without streaming stream is finished already
                if ( _onlyAsFile ) {
                    kdDebug() << "local file AS_FILE_ONLY" << endl;
                    finish( false );
                }
            } else {
                kdDebug() << "remote file" << endl;

                // stream into temporary file (use lower() in case the
                // filename as an upper case X in it)
                _tempFile = new KTempFile;
                _tempFile->setAutoDelete( TRUE );
                _fileURL = _tempFile->name();
                kdDebug() << "saving into " << _fileURL << endl;
            }
        }
    }

}

bool NSPluginStreamBase::create( const QString& url, const QString& mimeType, void *notify, bool forceNotify)
{
    if ( _stream )
        return false;

    _url = url;
    _notifyData = notify;
    _pos = 0;
    _tries = 0;
    _onlyAsFile = false;
    _streamType = NP_NORMAL;
    _informed = false;
    _forceNotify = forceNotify;

    // create new stream
    _stream = new NPStream;
    _stream->ndata = this;
    _stream->url = strdup(url.ascii());
    _stream->end = 0;
    _stream->pdata = 0;
    _stream->lastmodified = 0;
    _stream->notifyData = _notifyData;

    _mimeType = mimeType;

    return true;
}


int NSPluginStreamBase::process( const QByteArray &data, int start )
{
   int32 max, sent, to_sent, len;
   char *d = data.data() + start;

   to_sent = data.size() - start;
   while (to_sent > 0)
   {
      inform();

      max = _instance->NPWriteReady(_stream);
      //kdDebug(1431) << "to_sent == " << to_sent << " and max = " << max << endl;
      len = QMIN(max, to_sent);

      //kdDebug(1431) << "-> Feeding stream to plugin: offset=" << _pos << ", len=" << len << endl;
      sent = _instance->NPWrite( _stream, _pos, len, d );
      //kdDebug(1431) << "<- Feeding stream: sent = " << sent << endl;

      if (sent == 0) // interrupt the stream for a few ms
          break;

      if (sent < 0) {
          // stream data rejected/error
          kdDebug(1431) << "stream data rejected/error" << endl;
          _error = true;
          break;
      }

      if (_tempFile) {
          _tempFile->dataStream()->writeRawBytes(d, sent);
      }

      to_sent -= sent;
      _pos += sent;
      d += sent;
   }

   return data.size() - to_sent;
}


bool NSPluginStreamBase::pump()
{
    //kdDebug(1431) << "queue pos " << _queuePos << ", size " << _queue.size() << endl;

    inform();

    if ( _queuePos<_queue.size() ) {
        unsigned newPos;

        // handle AS_FILE_ONLY streams
        if ( _onlyAsFile ) {
            if (_tempFile) {
                _tempFile->dataStream()->writeRawBytes( _queue, _queue.size() );
	    }
            newPos = _queuePos+_queue.size();
        } else {
            // normal streams
            newPos = process( _queue, _queuePos );
        }

        // count tries
        if ( newPos==_queuePos )
            _tries++;
        else
            _tries = 0;

        _queuePos = newPos;
    }

    // return true if queue finished
    return _queuePos>=_queue.size();
}


void NSPluginStreamBase::queue( const QByteArray &data )
{
    _queue = data;
    _queue.detach();
    _queuePos = 0;
    _tries = 0;

/*
    kdDebug(1431) << "new queue size=" << data.size()
                  << " data=" << (void*)data.data() 
                  << " queue=" << (void*)_queue.data() << " qsize="
                  << _queue.size() << endl;
*/
}


void NSPluginStreamBase::finish( bool err )
{
    kdDebug(1431) << "finish error=" << err << endl;

    _queue.resize( 0 );
    _pos = 0;
    _queuePos = 0;

    inform();

    if ( !err ) {
        if ( _tempFile ) {
            _tempFile->close();
            _instance->addTempFile( _tempFile );
            _tempFile = 0;
        }

        if ( !_fileURL.isEmpty() ) {
            kdDebug() << "stream as file " << _fileURL << endl;
             _instance->NPStreamAsFile( _stream, _fileURL.ascii() );
        }

        _instance->NPDestroyStream( _stream, NPRES_DONE );
        if (_notifyData || _forceNotify)
            _instance->NPURLNotify( _url.url(), NPRES_DONE, _notifyData );
    } else {
        // close temp file
        if ( _tempFile ) {
            _tempFile->close();
	}

        // destroy stream
        _instance->NPDestroyStream( _stream, NPRES_NETWORK_ERR );
        if (_notifyData || _forceNotify)
            _instance->NPURLNotify( _url.url(), NPRES_NETWORK_ERR, _notifyData );
    }

    // delete stream
    if (_stream && _stream->url)
        free(const_cast<char *>(_stream->url));
    delete _stream;
    _stream = 0;

    // destroy NSPluginStream object
    emit finished( this );
}


/****************************************************************************/

NSPluginBufStream::NSPluginBufStream( class NSPluginInstance *instance )
    : NSPluginStreamBase( instance )
{
    _timer = new QTimer( this );
    connect( _timer, SIGNAL(timeout()), this, SLOT(timer()) );
}


NSPluginBufStream::~NSPluginBufStream()
{

}


bool NSPluginBufStream::get( const QString& url, const QString& mimeType,
                             const QByteArray &buf, void *notifyData,
                             bool singleShot )
{
    _singleShot = singleShot;
    if ( create( url, mimeType, notifyData ) ) {
        queue( buf );
        _timer->start( 100, true );
    }

    return false;
}


void NSPluginBufStream::timer()
{
    bool finished = pump();
    if ( _singleShot )
        finish( false );
    else {

        if ( !finished && tries()<=8 )
            _timer->start( 100, true );
        else
            finish( error() || tries()>8 );
    }
}



/****************************************************************************/

NSPluginStream::NSPluginStream( NSPluginInstance *instance )
    : NSPluginStreamBase( instance ), _job(0)
{
   _resumeTimer = new QTimer( this );
   connect(_resumeTimer, SIGNAL(timeout()), this, SLOT(resume()));
}


NSPluginStream::~NSPluginStream()
{
    if ( _job )
        _job->kill( true );
}


bool NSPluginStream::get( const QString& url, const QString& mimeType,
                          void *notify, bool reload )
{
    // create new stream
    if ( create( url, mimeType, notify ) ) {
        // start the kio job
        _job = KIO::get(KURL( url ), false, false);
        _job->addMetaData("errorPage", "false");
        _job->addMetaData("AllowCompressedPage", "false");
        if (reload) {
            _job->addMetaData("cache", "reload");
        }
        connect(_job, SIGNAL(data(KIO::Job *, const QByteArray &)),
                SLOT(data(KIO::Job *, const QByteArray &)));
        connect(_job, SIGNAL(result(KIO::Job *)), SLOT(result(KIO::Job *)));
        connect(_job, SIGNAL(totalSize(KIO::Job *, KIO::filesize_t )),
                SLOT(totalSize(KIO::Job *, KIO::filesize_t)));
        connect(_job, SIGNAL(mimetype(KIO::Job *, const QString &)),
                SLOT(mimetype(KIO::Job *, const QString &)));
    }

    return false;
}


bool NSPluginStream::post( const QString& url, const QByteArray& data, 
           const QString& mimeType, void *notify, const KParts::URLArgs& args )
{
    // create new stream
    if ( create( url, mimeType, notify ) ) {
        // start the kio job
        _job = KIO::http_post(KURL( url ), data, false);
        _job->addMetaData("content-type", args.contentType());
        _job->addMetaData("errorPage", "false");
        _job->addMetaData("AllowCompressedPage", "false");
        connect(_job, SIGNAL(data(KIO::Job *, const QByteArray &)),
                SLOT(data(KIO::Job *, const QByteArray &)));
        connect(_job, SIGNAL(result(KIO::Job *)), SLOT(result(KIO::Job *)));
        connect(_job, SIGNAL(totalSize(KIO::Job *, KIO::filesize_t )),
                SLOT(totalSize(KIO::Job *, KIO::filesize_t)));
        connect(_job, SIGNAL(mimetype(KIO::Job *, const QString &)),
                SLOT(mimetype(KIO::Job *, const QString &)));
    }

    return false;
}


void NSPluginStream::data(KIO::Job * job, const QByteArray &data)
{
    //kdDebug(1431) << "NSPluginStream::data - job=" << (void*)job << " data size=" << data.size() << endl;
    queue( data );
    if ( !pump() ) {
        _job->suspend();
        _resumeTimer->start( 100, TRUE );
    }
}

void NSPluginStream::totalSize(KIO::Job * job, KIO::filesize_t size)
{
    kdDebug(1431) << "NSPluginStream::totalSize - job=" << (void*)job << " size=" << KIO::number(size) << endl;
    _stream->end = size;
}

void NSPluginStream::mimetype(KIO::Job * job, const QString &mimeType)
{
    kdDebug(1431) << "NSPluginStream::QByteArray - job=" << (void*)job << " mimeType=" << mimeType << endl;
    _mimeType = mimeType;
}




void NSPluginStream::resume()
{
   if ( error() || tries()>8 ) {
       _job->kill( true );
       finish( true );
       return;
   }

   if ( pump() ) {
      kdDebug(1431) << "resume job" << endl;
      _job->resume();
   } else {
       kdDebug(1431) << "restart timer" << endl;
       _resumeTimer->start( 100, TRUE );
   }
}


void NSPluginStream::result(KIO::Job *job)
{
   int err = job->error();
   _job = 0;
   finish( err!=0 || error() );
}

#include "nsplugin.moc"
// vim: ts=4 sw=4 et
