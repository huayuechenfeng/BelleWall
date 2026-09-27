#ifndef BELLEWALL_WEBSTREAM_H
#define BELLEWALL_WEBSTREAM_H
#include "webtransport.h"
#include "sywpstorage.h"
class WebStream:public QObject{
public:
    WebStream():frames(0),busy(false),lastMs(0),published(0),timer(0),wasPaused(false),nextReportMs(0){life.start();}
    ~WebStream(){owner.Close();mutex.Close();chunk.Close();}
    void startL(){User::LeaveIfError(chunk.OpenGlobal(KWebChunk,EFalse));if(chunk.Size()<sizeof(TWebFrames)||chunk.Size()>1024*1024)User::Leave(KErrCorrupt);User::LeaveIfError(mutex.OpenGlobal(KWebMutex));frames=reinterpret_cast<TWebFrames*>(chunk.Base());if(frames->magic!=KWebMagic)User::Leave(KErrCorrupt);User::LeaveIfError(owner.Open(TProcessId(frames->owner)));if(owner.SecureId().iId!=0xe7b31103||owner.ExitType()!=EExitPending)User::Leave(KErrPermissionDenied);if(frames->nonceLo||frames->nonceHi){TBuf<128> args;User::CommandLine(args);TCandidateRecord r;Mem::FillZ(&r,sizeof(r));r.nonceLo=frames->nonceLo;r.nonceHi=frames->nonceHi;if(!CandidateArgsMatch(r,args))User::Leave(KErrPermissionDenied);}QString html="C:/data/BelleWall/animation.html";QFile selection(QFile::exists("C:/data/BelleWall/selected-wallpaper.txt")?"C:/data/BelleWall/selected-wallpaper.txt":"C:/data/BelleWall/selected-web.txt");if(selection.exists()){if(!selection.open(QIODevice::ReadOnly))User::Leave(KErrAccessDenied);QString name=SywpStorageId(QString::fromLatin1(selection.readAll()));if(name.isEmpty()||!name.endsWith(".html"))User::Leave(KErrCorrupt);html=SywpStoragePath(name);}content.loadL(html);content.resumeL(QSize(180,320));timer=startTimer(100);logLine("STREAM opened shared RGB565 double buffer");}
protected:
    void timerEvent(QTimerEvent*){
        if(busy)return;
        if(mutex.Wait(100000)!=KErrNone){qApp->exit(KErrTimedOut);return;}bool stop=frames->stop,paused=frames->paused;mutex.Signal();
        if(stop||owner.ExitType()!=EExitPending||(frames->limitMs>0&&life.elapsed()>frames->limitMs)){logLine(QString("STREAM stopped frames=%1").arg(published));qApp->quit();return;}
        if(paused){if(!wasPaused){content.pause();killTimer(timer);timer=startTimer(500);wasPaused=true;}lastMs=life.elapsed();return;}
        if(wasPaused){TRAPD(resume,content.resumeL(QSize(180,320)));if(resume){qApp->exit(resume);return;}killTimer(timer);timer=startTimer(100);lastMs=life.elapsed();wasPaused=false;}
        if(!content.ready())return;
        busy=true;TRAPD(error,renderL());busy=false;if(error){logLine(QString("STREAM error=%1").arg(error));qApp->exit(error);}
    }
private:
    void renderL(){
        const qint64 now=life.elapsed();const int delta=lastMs&&now-lastMs<=5000?int(now-lastMs):100;lastMs=now;QElapsedTimer timing;timing.start();
        const QImage& image=content.frameL(delta);if(image.format()!=QImage::Format_RGB16||image.width()!=180||image.height()!=320)User::Leave(KErrCorrupt);
        const int renderUs=int(timing.elapsed()*1000);timing.restart();User::LeaveIfError(mutex.Wait(100000));
        const int slot=1-(frames->slot&1);for(int y=0;y<320;y++)Mem::Copy(frames->pixels[slot]+y*360,image.constScanLine(y),360);
        frames->slot=slot;frames->renderUs=renderUs;frames->copyUs=int(timing.elapsed()*1000);frames->sequence=frames->sequence==KMaxTInt?1:frames->sequence+1;mutex.Signal();published++;
        if((frames->limitMs&&published%20==0)||(!frames->limitMs&&now>=nextReportMs)){nextReportMs=now+60000;logLine(QString("STREAM frames=%1 render_us=%2").arg(published).arg(renderUs));}
    }
    RProcess owner;RChunk chunk;RMutex mutex;TWebFrames* frames;WebContent content;QElapsedTimer life;bool busy;qint64 lastMs;qint64 published;int timer;bool wasPaused;qint64 nextReportMs;
};
#endif
