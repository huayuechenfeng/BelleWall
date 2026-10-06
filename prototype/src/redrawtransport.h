#ifndef BELLEWALL_REDRAW_TRANSPORT_H
#define BELLEWALL_REDRAW_TRANSPORT_H
#include <e32base.h>
#include <coecntrl.h>
#include "candidatesession.h"
#include "displaypolicy.h"
#include "renderreadiness.h"
_LIT(KRedrawChunk,"BelleWallDirtyRectsV3");
_LIT(KRedrawMutex,"BelleWallDirtyRectsLockV3");
const TUint KRedrawMagic=0x33524442;
struct TRedrawFrame {TUint magic;TUint owner;TUint nonceLo;TUint nonceHi;TUint sequence;TUint acknowledged;TInt left;TInt top;TInt right;TInt bottom;TInt width,height,observedWidth,observedHeight;TUint geometry,targetGeometry;TUint consumerProtocol;};
class CRedrawPublisher:public CBase {
public:
    static CRedrawPublisher* NewL(){CRedrawPublisher* self=new(ELeave)CRedrawPublisher;CleanupStack::PushL(self);User::LeaveIfError(self->iMutex.CreateGlobal(KRedrawMutex));User::LeaveIfError(self->iChunk.CreateGlobal(KRedrawChunk,sizeof(TRedrawFrame),sizeof(TRedrawFrame)));self->iFrame=reinterpret_cast<TRedrawFrame*>(self->iChunk.Base());Mem::FillZ(self->iFrame,sizeof(TRedrawFrame));self->iFrame->owner=RProcess().Id().Id();RChunk live;TCandidateShared* session=CandidateOpen(live);if(session){self->iFrame->nonceLo=session->record.nonceLo;self->iFrame->nonceHi=session->record.nonceHi;}live.Close();self->iFrame->magic=KRedrawMagic;CleanupStack::Pop(self);return self;}
    ~CRedrawPublisher(){iChunk.Close();iMutex.Close();}
    TBool BeginWrite(){if(iMutex.Wait(100000)!=KErrNone)return EFalse;if(!BelleDisplay::CurrentGeometry(iFrame->geometry,iFrame->targetGeometry,iFrame->observedWidth,iFrame->observedHeight,iFrame->width,iFrame->height)){iMutex.Signal();return EFalse;}return ETrue;}
    void EndWrite(){iMutex.Signal();}
    void TargetL(const TSize& size){TargetL(size,GeometryL());}
    TUint GeometryL(){User::LeaveIfError(iMutex.Wait(100000));TUint epoch=iFrame->geometry;iMutex.Signal();return epoch;}
    void TargetL(const TSize& size,TUint epoch){User::LeaveIfError(iMutex.Wait(100000));iFrame->width=size.iWidth;iFrame->height=size.iHeight;iFrame->targetGeometry=epoch;iFrame->acknowledged=iFrame->sequence;if(!iFrame->observedWidth){iFrame->observedWidth=size.iWidth;iFrame->observedHeight=size.iHeight;}iMutex.Signal();}
    TUint ConsumerProtocolL(){User::LeaveIfError(iMutex.Wait(100000));TUint protocol=iFrame->consumerProtocol;iMutex.Signal();return protocol;}
    TBool NeedsRebuildL(){User::LeaveIfError(iMutex.Wait(100000));TBool changed=iFrame->geometry!=iFrame->targetGeometry;iMutex.Signal();return changed;}
    TSize ObservedL(){User::LeaveIfError(iMutex.Wait(100000));TSize size(iFrame->observedWidth,iFrame->observedHeight);iMutex.Signal();return size;}
    void Publish(const TRect& dirty){User::LeaveIfError(iMutex.Wait(100000));TRect r(dirty);r.Intersection(TRect(TPoint(0,0),TSize(iFrame->width,iFrame->height)));if(r.IsEmpty()||!BelleDisplay::CurrentGeometry(iFrame->geometry,iFrame->targetGeometry,iFrame->observedWidth,iFrame->observedHeight,iFrame->width,iFrame->height)){iMutex.Signal();return;}if(iFrame->sequence!=iFrame->acknowledged)r.BoundingRect(TRect(iFrame->left,iFrame->top,iFrame->right,iFrame->bottom));iFrame->left=r.iTl.iX;iFrame->top=r.iTl.iY;iFrame->right=r.iBr.iX;iFrame->bottom=r.iBr.iY;iFrame->sequence++;iMutex.Signal();}
private:
    CRedrawPublisher():iFrame(0){}RMutex iMutex;RChunk iChunk;TRedrawFrame* iFrame;
};
static void NotifyDesktopLayoutChanged(){RMutex mutex;if(mutex.OpenGlobal(KRedrawMutex))return;RChunk chunk;if(chunk.OpenGlobal(KRedrawChunk,EFalse)){mutex.Close();return;}if(chunk.Size()>=TInt(sizeof(TRedrawFrame))&&mutex.Wait(1000)==KErrNone){TRedrawFrame* frame=reinterpret_cast<TRedrawFrame*>(chunk.Base());if(frame->magic==KRedrawMagic){frame->geometry++;frame->acknowledged=frame->sequence;}mutex.Signal();}chunk.Close();mutex.Close();}
// Open and close per call: never keep the publisher's chunk alive after exit.
// Acknowledgement means DrawNow was issued for that region, not LCD presentation.
static TInt InvalidatePublishedRect(CCoeControl& bg,RWsSession& ws,TInt& area){
    area=0;RMutex mutex;TInt error=mutex.OpenGlobal(KRedrawMutex);if(error==KErrNotFound)return 0;if(error)return error;
    RChunk chunk;error=chunk.OpenGlobal(KRedrawChunk,EFalse);if(error){mutex.Close();return error==KErrNotFound?0:error;}
    if(chunk.Size()<TInt(sizeof(TRedrawFrame))){chunk.Close();mutex.Close();return KErrCorrupt;}
    if(mutex.Wait(1)!=KErrNone){chunk.Close();mutex.Close();return 0;}TRedrawFrame* frame=reinterpret_cast<TRedrawFrame*>(chunk.Base());TInt result=0;
    RChunk live;TCandidateShared* session=CandidateOpen(live);TBool running=session&&session->record.state==ERunning;TBool own=session&&!session->stop&&session->record.owner==frame->owner&&session->record.nonceLo==frame->nonceLo&&session->record.nonceHi==frame->nonceHi;live.Close();
    if(own&&frame->magic==KRedrawMagic&&BelleDisplay::Valid(bg.Size().iWidth,bg.Size().iHeight)&&bg.OwnsWindow())frame->consumerProtocol=BelleRenderReadiness::Protocol;
    if(!own)result=0;else if(frame->magic==0)result=0;else if(frame->magic!=KRedrawMagic)result=KErrCorrupt;
    else if(!BelleDisplay::Valid(bg.Size().iWidth,bg.Size().iHeight)||!bg.OwnsWindow())result=0;
    else if(frame->observedWidth!=bg.Size().iWidth||frame->observedHeight!=bg.Size().iHeight){frame->observedWidth=bg.Size().iWidth;frame->observedHeight=bg.Size().iHeight;frame->geometry++;frame->acknowledged=frame->sequence;}
    else if(running&&BelleDisplay::CurrentGeometry(frame->geometry,frame->targetGeometry,bg.Size().iWidth,bg.Size().iHeight,frame->width,frame->height)&&frame->sequence!=frame->acknowledged){
        TRect dirty(frame->left,frame->top,frame->right,frame->bottom);
        if(dirty.IsEmpty()||dirty.iTl.iX<0||dirty.iTl.iY<0||dirty.iBr.iX>frame->width||dirty.iBr.iY>frame->height)result=KErrCorrupt;
        else {bg.DrawNow(dirty);ws.Flush();frame->acknowledged=frame->sequence;area=dirty.Width()*dirty.Height();result=1;}
    }
    mutex.Signal();chunk.Close();mutex.Close();return result;
}
#endif
