#ifndef BELLEWALL_HOST_H
#define BELLEWALL_HOST_H
#include <QtCore/QSettings>
#include <QtCore/QTimerEvent>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <e32property.h>
#include <w32std.h>
#include <apgwgnam.h>
#include <eikenv.h>
#include <alfcompositionutility.h>
#include <videoplayer2.h>
#include <hwrmlight.h>
#include <avkondomainpskeys.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include "webcontent.h"

class GateSink {public:virtual void gatesChanged()=0;};
class CompositionResearchAccess : public CAlfCompositionClientBase {
public:using CAlfCompositionClientBase::SendEvent;
};
inline TInt legacyBackground(CAlfCompositionSource& source,TBool enabled){
    // Invoke the original, already-declared member through a public member pointer.
    // No object cast, DLL patch, or replacement. 15011 is the old source protocol.
    TInt (CAlfCompositionClientBase::*send)(TInt,const TAny*,TInt)=&CompositionResearchAccess::SendEvent;
    return (source.*send)(15011,&enabled,sizeof(enabled));
}
class PropertyWatch : public CActive {
public:
    PropertyWatch(GateSink& sink):CActive(EPriorityHigh),value(-1),error(KErrNotReady),sink(sink){CActiveScheduler::Add(this);}
    ~PropertyWatch(){Cancel();property.Close();}
    void attachL(TUid category,TUint key){User::LeaveIfError(property.Attach(category,key));subscribe();read();}
    void read(){error=property.Get(value);if(error)value=-1;}
    int value,error;
private:
    void subscribe(){property.Subscribe(iStatus);SetActive();}
    void RunL(){const TInt status=iStatus.Int();if(status==KErrNone){subscribe();read();}else{error=status;value=-1;}sink.gatesChanged();}
    void DoCancel(){property.Cancel();}
    RProperty property;GateSink& sink;
};

class Host : public QObject, public MAlfCompositionObserver, public MVideoPlayerUtilityObserver,
             public MHWRMLightObserver, public GateSink {
public:
    Host():screen(0),name(0),source(0),video(0),light(0),hs(0),lock(0),display(EGL_NO_DISPLAY),surface(EGL_NO_SURFACE),context(EGL_NO_CONTEXT),texture(0),pollTimer(0),frameTimer(0),ready(false),running(false),playing(false),targetVisible(true),failed(false),fps(10),maxSeconds(180),ticks(0),lastGate(-1),experimentalLegacy(false),focusUid(0){}
    ~Host(){
        if(pollTimer)killTimer(pollTimer);if(frameTimer)killTimer(frameTimer);
        delete hs;delete lock;delete light;
        web.pause();if(video){video->Stop();video->Close();delete video;}
        if(source){source->RemoveObserver(*this);if(experimentalLegacy)logLine(QString("LEGACY unregister=%1").arg(legacyBackground(*source,EFalse)));source->SetIsBackgroundAnim(EFalse);delete source;}
        releaseGL();window.Close();group.Close();delete name;delete screen;ws.Close();
        logLine("STOP: source/window released; original wallpaper settings untouched");
    }
    void startL(){
        QSettings c("C:/data/BelleWall/config.ini",QSettings::IniFormat);
        mode=c.value("mode","blocks").toString();file=c.value("file").toString();
        experimentalLegacy=c.value("experimentalLegacy",false).toBool();
        fps=qBound(1,c.value("fps",10).toInt(),30);maxSeconds=qBound(10,c.value("maxSeconds",180).toInt(),3600);
        if(mode!="blocks"&&mode!="video"&&mode!="web")User::Leave(KErrArgument);
        if(mode!="blocks"&&(!file.startsWith("C:/data/BelleWall/")&&!file.startsWith("E:/data/BelleWall/")))User::Leave(KErrArgument);
        logLine("START mode="+mode+" file="+file+"; runtime background validation pending");
        User::LeaveIfError(ws.Connect());screen=new(ELeave)CWsScreenDevice(ws);User::LeaveIfError(screen->Construct(0));
        group=RWindowGroup(ws);User::LeaveIfError(group.Construct(reinterpret_cast<TUint32>(this),EFalse));
        group.EnableReceiptOfFocus(EFalse);group.SetOrdinalPosition(-1,-1000);
        name=CApaWindowGroupName::NewL(ws);name->SetAppUid(TUid::Uid(0xe7b31101));name->SetHidden(ETrue);name->SetSystem(ETrue);name->SetCaptionL(_L("BelleWall background"));name->SetWindowGroupName(group);
        window=RWindow(ws);User::LeaveIfError(window.Construct(group,1));
        resizeL();window.SetVisible(EFalse);window.Activate();ws.Flush();
        hs=new(ELeave)PropertyWatch(*this);lock=new(ELeave)PropertyWatch(*this);
        // These identifiers are in the supplied Nokia source / SDK, not guessed ROM ordinals.
        hs->attachL(TUid::Uid(KUidSystemCategoryValue),0x2002ea91);
        lock->attachL(KPSUidAvkonDomain,KAknKeyguardStatus);
        light=CHWRMLight::NewL(this);
        if(mode=="video"){
            video=CVideoPlayerUtility2::NewL(*this,EMdaPriorityMin,EMdaPriorityPreferenceNone);
            const TPtrC filename(reinterpret_cast<const TUint16*>(file.utf16()),file.size());video->OpenFileL(filename);
        }else{
            if(mode=="web")web.loadL(file);
            initGLL();renderL();bindL();ready=true;
        }
        session.start();pollTimer=startTimer(250);gatesChanged();
    }
    void gatesChanged(){TRAPD(err,updateL());if(err)fail("lifecycle",err);}
    void FrameReady(TInt){}
    void CompositionTargetHidden(){targetVisible=false;gatesChanged();}
    void CompositionTargetVisible(){targetVisible=true;gatesChanged();}
    void RunningLowOnGraphicsMemory(){fail("graphics memory",KErrNoMemory);}
    void LightStatusChanged(TInt,CHWRMLight::TLightStatus){gatesChanged();}
    void MvpuoOpenComplete(TInt err){
        logLine(QString("MMF open=%1").arg(err));if(err){fail("video open",err);return;}
        TRAP(err,video->AddDisplayWindowL(ws,*screen,window,TRect(renderSize),TRect(renderSize));video->SetAutoScaleL(window,EAutoScaleBestFit));
        if(err){fail("video surface attach",err);return;}video->Prepare();
    }
    void MvpuoPrepareComplete(TInt err){
        logLine(QString("MMF prepare=%1").arg(err));if(err){fail("video prepare",err);return;}
        TRAP(err,video->SetVolumeL(0);bindL());if(err){fail("video bind",err);return;}
        ready=true;gatesChanged();
    }
    void MvpuoPlayComplete(TInt err){
        playing=false;logLine(QString("MMF end=%1").arg(err));
        if(err){fail("video decode",err);return;}
        TRAP(err,video->SetPositionL(TTimeIntervalMicroSeconds(0)));if(err){fail("video rewind",err);return;}
        if(running){video->Play();playing=true;logLine("MMF loop restart");}
    }
    void MvpuoFrameReady(CFbsBitmap&,TInt){}
    void MvpuoEvent(const TMMFEvent& event){if(event.iErrorCode)fail("MMF event",event.iErrorCode);}
protected:
    void timerEvent(QTimerEvent* e){
        if(failed)return;
        if(e->timerId()==pollTimer){
            QSettings c("C:/data/BelleWall/config.ini",QSettings::IniFormat);
            if(!c.value("enabled",true).toBool()||QFile::exists("C:/data/BelleWall/stop")||session.elapsed()>maxSeconds*1000){QApplication::quit();return;}
            if(c.value("mode","blocks").toString()!=mode||c.value("file").toString()!=file){logLine("Config content changed: stopping; relaunch to switch");QApplication::quit();return;}
            hs->read();lock->read();gatesChanged();
            const TSize size=screen->SizeInPixels();
            if(size!=displaySize){TRAPD(err,rotateL());if(err)fail("rotation",err);}
            if(!ready&&session.elapsed()>15000)fail("load timeout",KErrTimedOut);
        }else if(e->timerId()==frameTimer&&running){TRAPD(err,renderL());if(err)fail("render",err);}
    }
private:
    void fail(const char* where,TInt err){
        if(failed)return;failed=true;ready=false;
        if(frameTimer){killTimer(frameTimer);frameTimer=0;}web.pause();if(video)video->Stop();
        window.SetVisible(EFalse);ws.Flush();logLine(QString("FAIL %1 code=%2; exiting to static wallpaper").arg(where).arg(err));QApplication::exit(err?err:1);
        // Also handles a failure before app.exec() has entered its event loop.
        QTimer::singleShot(0,QApplication::instance(),SLOT(quit()));
    }
    void updateL(){
        if(!hs||!lock||!light||failed)return;
        // ALF's HS property also becomes zero when wallpaper occludes animation.
        // Record independent focus evidence; never bypass the rendering gates.
        const TInt focus=ws.GetFocusWindowGroup();
        TInt view=-1;const TInt viewError=RProperty::Get(TUid::Uid(0x20022f35),1,view);
        const QString focusState=QString("%1/%2/%3").arg(focus).arg(view).arg(viewError);
        if(focusState!=lastFocusState){
            lastFocusState=focusState;TUint uid=0;
            TRAPD(focusError,CApaWindowGroupName* fg=CApaWindowGroupName::NewL(ws,focus);uid=fg->AppUid().iUid;delete fg;);focusUid=uid;
            logLine(QString("FOCUS wg=%1 uid=%2 error=%3 HSview=%4/%5").arg(focus).arg(uid,8,16,QChar('0')).arg(focusError).arg(view).arg(viewError));
        }
        const int lights=light->LightStatus(CHWRMLight::EPrimaryDisplay);
        const bool desktop=experimentalLegacy?(focusUid==0x102750f0):(hs->error==KErrNone&&hs->value!=0);
        const bool allow=ready&&targetVisible&&desktop&&lock->error==KErrNone&&lock->value==EKeyguardNotActive&&lights==CHWRMLight::ELightOn;
        const int bits=(ready?1:0)|(targetVisible?2:0)|((hs->value!=0&&hs->error==0)?4:0)|((lock->value==0&&lock->error==0)?8:0)|(lights==CHWRMLight::ELightOn?16:0);
        if(bits!=lastGate){lastGate=bits;logLine(QString("GATE ready/target/HS/unlocked/light=%1 HS=%2/%3 lock=%4/%5 light=%6").arg(bits).arg(hs->value).arg(hs->error).arg(lock->value).arg(lock->error).arg(lights));}
        if(allow==running)return;
        running=allow;
        if(!allow){
            if(frameTimer){killTimer(frameTimer);frameTimer=0;}
            web.pause();if(video&&playing){video->PauseL();playing=false;}
            // Keep the registered source alive for ALF visibility callbacks. No frame updates.
            logLine("PAUSE: decoding/HTML page/frame timer stopped");
        }else{
            if(mode=="web")web.resumeL(QSize(renderSize.iWidth,renderSize.iHeight));
            window.SetVisible(ETrue);ws.Flush();
            if(video){video->Play();playing=true;}else frameTimer=startTimer(1000/fps);
            logLine("RESUME");
        }
    }
    void resizeL(){displaySize=screen->SizeInPixels();renderSize=(mode=="video")?displaySize:TSize(qMax(1,displaySize.iWidth/2),qMax(1,displaySize.iHeight/2));User::LeaveIfError(window.SetExtentErr(TPoint(0,0),renderSize));}
    void rotateL(){
        logLine("ROTATE: resizing surface");bool was=running;running=false;
        if(frameTimer){killTimer(frameTimer);frameTimer=0;}web.pause();
        if(video&&playing){video->PauseL();playing=false;}
        if(!video)releaseGL();resizeL();
        if(video){if(ready){video->SetVideoExtentL(window,TRect(renderSize));video->SetWindowClipRectL(window,TRect(renderSize));}}
        else initGLL();
        if(source)User::LeaveIfError(source->SetExtent(TRect(displaySize),0));
        Q_UNUSED(was);updateL();
    }
    void bindL(){
        source=CAlfCompositionSource::NewL(window);source->AddCompositionObserverL(*this);
        source->SetIsBackgroundAnim(ETrue);User::LeaveIfError(source->SetExtent(TRect(displaySize),0));
        if(experimentalLegacy){const TInt result=legacyBackground(*source,ETrue);logLine(QString("LEGACY register operation=15011 result=%1; experimental focus gate, not display proof").arg(result));User::LeaveIfError(result);}
        window.SetVisible(ETrue);ws.Flush();
        logLine("ALF NewL/observer/extent completed; SetIsBackgroundAnim is void, NOT proof of native background visibility");
    }
    void initGLL(){
        display=eglGetDisplay(EGL_DEFAULT_DISPLAY);EGLint major,minor,count;
        if(display==EGL_NO_DISPLAY||!eglInitialize(display,&major,&minor))User::Leave(KErrNotSupported);
        const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES_BIT,EGL_RED_SIZE,5,EGL_GREEN_SIZE,6,EGL_BLUE_SIZE,5,EGL_NONE};
        EGLConfig config;if(!eglChooseConfig(display,attrs,&config,1,&count)||count!=1)User::Leave(KErrNotSupported);
        if(!eglBindAPI(EGL_OPENGL_ES_API))User::Leave(KErrNotSupported);
        surface=eglCreateWindowSurface(display,config,&window,0);context=eglCreateContext(display,config,EGL_NO_CONTEXT,0);
        if(surface==EGL_NO_SURFACE||context==EGL_NO_CONTEXT||!eglMakeCurrent(display,surface,surface,context))User::Leave(KErrNotSupported);
        glViewport(0,0,renderSize.iWidth,renderSize.iHeight);
        if(mode=="web"){
            texWidth=1;texHeight=1;while(texWidth<renderSize.iWidth)texWidth*=2;while(texHeight<renderSize.iHeight)texHeight*=2;
            glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,texWidth,texHeight,0,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,0);
        }
    }
    void releaseGL(){
        if(display!=EGL_NO_DISPLAY){eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(context!=EGL_NO_CONTEXT)eglDestroyContext(display,context);if(surface!=EGL_NO_SURFACE)eglDestroySurface(display,surface);eglTerminate(display);}
        display=EGL_NO_DISPLAY;surface=EGL_NO_SURFACE;context=EGL_NO_CONTEXT;texture=0;
    }
    void renderL(){
        if(!eglMakeCurrent(display,surface,surface,context))User::Leave(KErrNotReady);
        glClearColor(0.03f,0.09f,0.16f,1);glClear(GL_COLOR_BUFFER_BIT);
        if(mode=="blocks"){
            glEnable(GL_SCISSOR_TEST);const int size=qMax(8,renderSize.iWidth/5);
            glScissor((ticks*3)%qMax(1,renderSize.iWidth-size),renderSize.iHeight/2,size,size);glClearColor(0.1f,0.8f,0.6f,1);glClear(GL_COLOR_BUFFER_BIT);glDisable(GL_SCISSOR_TEST);
        }else if(mode=="web"&&running){
            const QImage& im=web.frameL(1000/fps);glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);glPixelStorei(GL_UNPACK_ALIGNMENT,4);
            glTexSubImage2D(GL_TEXTURE_2D,0,0,0,im.width(),im.height(),GL_RGB,GL_UNSIGNED_SHORT_5_6_5,im.bits());
            const GLfloat vertices[]={-1,-1,1,-1,-1,1,1,1};const GLfloat u=GLfloat(im.width())/texWidth,v=GLfloat(im.height())/texHeight;
            const GLfloat uv[]={0,v,u,v,0,0,u,0};glEnableClientState(GL_VERTEX_ARRAY);glEnableClientState(GL_TEXTURE_COORD_ARRAY);glVertexPointer(2,GL_FLOAT,0,vertices);glTexCoordPointer(2,GL_FLOAT,0,uv);glDrawArrays(GL_TRIANGLE_STRIP,0,4);glDisable(GL_TEXTURE_2D);
        }
        if(glGetError()!=GL_NO_ERROR||!eglSwapBuffers(display,surface))User::Leave(KErrGeneral);
        ++ticks;if(ticks%100==0)logLine(QString("FRAMES=%1 (submitted, not display proof)").arg(ticks));
    }
    RWsSession ws;RWindowGroup group;RWindow window;CWsScreenDevice* screen;CApaWindowGroupName* name;
    CAlfCompositionSource* source;CVideoPlayerUtility2* video;CHWRMLight* light;PropertyWatch* hs;PropertyWatch* lock;
    EGLDisplay display;EGLSurface surface;EGLContext context;GLuint texture;int texWidth,texHeight;
    int pollTimer,frameTimer;bool ready,running,playing,targetVisible,failed;int fps,maxSeconds,ticks,lastGate;
    QString mode,file,lastFocusState;TSize renderSize,displaySize;WebContent web;QElapsedTimer session;
    bool experimentalLegacy;TUint focusUid;
};
#endif

