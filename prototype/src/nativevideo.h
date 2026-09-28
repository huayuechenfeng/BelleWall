#ifndef BELLEWALL_NATIVEVIDEO_H
#define BELLEWALL_NATIVEVIDEO_H
#include <videoplayer2.h>
#include <fbs.h>
#include <hal.h>
#include <mmf/common/mmfvideoenums.h>
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
    ~NativeVideoFrames(){Report();delete timer;delete video;delete bitmap;}
    void OpenL(){OpenL(_L("C:\\data\\BelleWall\\sample.mp4"));}
    void OpenL(const TDesC& path){User::LeaveIfError(HAL::Get(HALData::EFastCounterFrequency,frequency));if(frequency<=0)User::Leave(KErrNotSupported);error=KErrNotReady;video->OpenFileL(path);WaitL();error=KErrNotReady;video->Prepare();WaitL();duration=video->DurationL().Int64();if(duration<=0)User::Leave(KErrCorrupt);video->VideoFrameSizeL(size);rate=video->VideoFrameRateL();
        TRAPD(infoError,const CMMFControllerImplementationInformation& info=video->ControllerImplementationInformationL();TPtrC name(info.DisplayName().Left(70));TBuf<180> line;line.Format(_L("VIDEO controller uid=%08x name=%S; hardware acceleration unverified"),info.Uid().iUid,&name);Log(line));if(infoError)Log(_L("VIDEO controller metadata unavailable"));
        TVideoPlayRateCapabilities caps;Mem::FillZ(&caps,sizeof(caps));TRAPD(capError,video->GetPlayRateCapabilitiesL(caps));
        if(!capError&&caps.iStepForward){TRAPD(pauseError,video->SetVolumeL(0);video->Play();video->PauseL());if(!pauseError)stepping=ETrue;else video->Stop();}
        Log(stepping?_L("VIDEO forward frame stepping enabled with position verification"):_L("VIDEO seek fallback; controller has no usable frame stepping"));
    }
    TSize FrameSize()const{return size;}
    TReal32 FrameRate()const{return rate;}
    TInt64 DurationUs()const{return duration;}
    CFbsBitmap& FrameL(TInt second){return FrameAtUsL(TInt64(second)*1000000);}
    CFbsBitmap& FrameAtUsL(TInt64 positionUs){positionUs%=duration;TUint start=User::FastCounter();TBool positioned=EFalse;
        if(stepping&&lastPosition>=0&&positionUs>lastPosition&&rate>=1){TInt steps=TInt((positionUs-lastPosition)*rate/1000000+0.5);if(steps>0&&steps<=120){TRAPD(stepError,video->StepFrameL(steps);TInt64 actual=video->PositionL().Int64();TInt64 delta=actual-positionUs;if(delta<0)delta=-delta;if(delta>TInt64(500000/rate))User::Leave(KErrNotReady));if(!stepError){positioned=ETrue;stepFrames++;}else {stepping=EFalse;Log(_L("VIDEO stepping rejected or inaccurate; using seek fallback"));}}}
        if(!positioned){video->SetPositionL(TTimeIntervalMicroSeconds(positionUs));seekFrames++;}lastPosition=positionUs;seekUs+=Elapsed(start);start=User::FastCounter();
        TRAPD(frameError,RequestFrameL());if((frameError==KErrNotSupported||frameError==KErrArgument)&&mode==EColor64K){mode=EColor16M;Log(_L("VIDEO RGB565 extraction unsupported; using RGB888"));RequestFrameL();}else User::LeaveIfError(frameError);waitUs+=Elapsed(start);if(!bitmap)User::Leave(KErrCorrupt);if(frames%300==0)Report();return *bitmap;
    }
    void MvpuoOpenComplete(TInt e){Done(e);}
    void MvpuoPrepareComplete(TInt e){Done(e);}
    void MvpuoFrameReady(CFbsBitmap& frame,TInt e){if(!e){frames++;if(!bitmap)bitmap=new CFbsBitmap;if(!bitmap)e=KErrNoMemory;else {bitmap->Reset();e=bitmap->Duplicate(frame.Handle());}}Done(e);}
    void MvpuoPlayComplete(TInt e){if(e)Done(e);}
    void MvpuoEvent(const TMMFEvent& event){if(event.iErrorCode)Done(event.iErrorCode);}
private:
    NativeVideoFrames():video(0),timer(0),bitmap(0),error(KErrNotReady),duration(0),size(0,0),rate(0),frames(0),frequency(0),mode(EColor64K),stepping(EFalse),lastPosition(-1),seekUs(0),waitUs(0),seekFrames(0),stepFrames(0){}
    TInt64 Elapsed(TUint start)const{return frequency?TInt64(TUint(User::FastCounter()-start))*1000000/frequency:0;}
    void RequestFrameL(){error=KErrNotReady;video->GetFrameL(mode);WaitL();}
    void Report(){TBuf<220> line;line.Format(_L("VIDEO profile frames=%d seek_frames=%d step_frames=%d position_us=%Ld frame_wait_us=%Ld mode=%d"),frames,seekFrames,stepFrames,seekUs,waitUs,mode);Log(line);}
    void Done(TInt e){error=e;if(wait.IsStarted())wait.AsyncStop();}
    void WaitL(){timer->After(10000000);if(error==KErrNotReady)wait.Start();timer->Cancel();User::LeaveIfError(error);}
    CVideoPlayerUtility2* video;FrameWaitTimeout* timer;CFbsBitmap* bitmap;CActiveSchedulerWait wait;TInt error;TInt64 duration;TSize size;TReal32 rate;TInt frames;
    TInt frequency;TDisplayMode mode;TBool stepping;TInt64 lastPosition,seekUs,waitUs;TInt seekFrames,stepFrames;
};
#endif
