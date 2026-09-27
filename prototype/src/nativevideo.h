#ifndef BELLEWALL_NATIVEVIDEO_H
#define BELLEWALL_NATIVEVIDEO_H
#include <videoplayer2.h>
#include <fbs.h>
class FrameWaitTimeout:public CTimer{
public:
    static FrameWaitTimeout* NewL(CActiveSchedulerWait& wait,TInt& error){FrameWaitTimeout* self=new(ELeave)FrameWaitTimeout(wait,error);CleanupStack::PushL(self);self->ConstructL();CleanupStack::Pop(self);return self;}
    FrameWaitTimeout(CActiveSchedulerWait& w,TInt& e):CTimer(EPriorityHigh),wait(w),error(e){CActiveScheduler::Add(this);}
    void RunL(){Log(_L("VIDEO async wait timeout"));error=KErrTimedOut;if(wait.IsStarted())wait.AsyncStop();}
private:CActiveSchedulerWait& wait;TInt& error;
};
class NativeVideoFrames:public CBase,public MVideoPlayerUtilityObserver{
public:
    static NativeVideoFrames* NewL(){NativeVideoFrames* self=new(ELeave)NativeVideoFrames;CleanupStack::PushL(self);self->timer=FrameWaitTimeout::NewL(self->wait,self->error);self->video=CVideoPlayerUtility2::NewL(*self,EMdaPriorityMin,EMdaPriorityPreferenceNone);CleanupStack::Pop(self);return self;}
    ~NativeVideoFrames(){Log(_L("VIDEO cleanup begin"));delete timer;delete video;delete bitmap;Log(_L("VIDEO cleanup end"));}
    void OpenL(){OpenL(_L("C:\\data\\BelleWall\\sample.mp4"));}
    void OpenL(const TDesC& path){error=KErrNotReady;video->OpenFileL(path);WaitL();error=KErrNotReady;video->Prepare();WaitL();duration=video->DurationL().Int64();if(duration<=0)User::Leave(KErrCorrupt);video->VideoFrameSizeL(size);rate=video->VideoFrameRateL();}
    TSize FrameSize()const{return size;}
    TReal32 FrameRate()const{return rate;}
    TInt64 DurationUs()const{return duration;}
    CFbsBitmap& FrameL(TInt second){return FrameAtUsL(TInt64(second)*1000000);}
    CFbsBitmap& FrameAtUsL(TInt64 positionUs){video->SetPositionL(TTimeIntervalMicroSeconds(positionUs%duration));delete bitmap;bitmap=0;error=KErrNotReady;video->GetFrameL(EColor16M);WaitL();if(!bitmap)User::Leave(KErrCorrupt);return *bitmap;}
    void MvpuoOpenComplete(TInt e){Done(e);}
    void MvpuoPrepareComplete(TInt e){Done(e);}
    void MvpuoFrameReady(CFbsBitmap& frame,TInt e){if(e||++frames==1||frames%300==0){TBuf<80> line;line.Format(_L("VIDEO FrameReady count=%d error=%d"),frames,e);Log(line);}if(!e){bitmap=new CFbsBitmap;if(!bitmap)e=KErrNoMemory;else e=bitmap->Duplicate(frame.Handle());}Done(e);}
    void MvpuoPlayComplete(TInt e){if(e)Done(e);}
    void MvpuoEvent(const TMMFEvent& event){if(event.iErrorCode)Done(event.iErrorCode);}
private:
    NativeVideoFrames():video(0),timer(0),bitmap(0),error(KErrNotReady),duration(0),size(0,0),rate(0),frames(0){}
    void Done(TInt e){error=e;if(wait.IsStarted())wait.AsyncStop();}
    void WaitL(){timer->After(10000000);if(error==KErrNotReady)wait.Start();timer->Cancel();User::LeaveIfError(error);}
    CVideoPlayerUtility2* video;FrameWaitTimeout* timer;CFbsBitmap* bitmap;CActiveSchedulerWait wait;TInt error;TInt64 duration;TSize size;TReal32 rate;TInt frames;
};
#endif
