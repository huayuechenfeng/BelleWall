#ifndef BELLEWALL_REDRAW_TRANSPORT_H
#define BELLEWALL_REDRAW_TRANSPORT_H
#include <e32base.h>
#include <coecntrl.h>
#include "candidatesession.h"
_LIT(KRedrawChunk,"BelleWallDirtyRectsV2");
_LIT(KRedrawMutex,"BelleWallDirtyRectsLockV2");
const TUint KRedrawMagic=0x32524442;
struct TRedrawFrame {TUint magic;TUint owner;TUint nonceLo;TUint nonceHi;TUint sequence;TUint acknowledged;TInt left;TInt top;TInt right;TInt bottom;};
class CRedrawPublisher:public CBase {
public:
    static CRedrawPublisher* NewL(){CRedrawPublisher* self=new(ELeave)CRedrawPublisher;CleanupStack::PushL(self);User::LeaveIfError(self->iMutex.CreateGlobal(KRedrawMutex));User::LeaveIfError(self->iChunk.CreateGlobal(KRedrawChunk,sizeof(TRedrawFrame),sizeof(TRedrawFrame)));self->iFrame=reinterpret_cast<TRedrawFrame*>(self->iChunk.Base());Mem::FillZ(self->iFrame,sizeof(TRedrawFrame));self->iFrame->owner=RProcess().Id().Id();RChunk live;TCandidateShared* session=CandidateOpen(live);if(session){self->iFrame->nonceLo=session->record.nonceLo;self->iFrame->nonceHi=session->record.nonceHi;}live.Close();self->iFrame->magic=KRedrawMagic;CleanupStack::Pop(self);return self;}
    ~CRedrawPublisher(){iChunk.Close();iMutex.Close();}
    TBool BeginWrite(){return iMutex.Wait(100000)==KErrNone;}
    void EndWrite(){iMutex.Signal();}
    void Publish(const TRect& dirty){TRect r(dirty);r.Intersection(TRect(0,0,360,640));if(r.IsEmpty())return;User::LeaveIfError(iMutex.Wait(100000));if(iFrame->sequence!=iFrame->acknowledged)r.BoundingRect(TRect(iFrame->left,iFrame->top,iFrame->right,iFrame->bottom));iFrame->left=r.iTl.iX;iFrame->top=r.iTl.iY;iFrame->right=r.iBr.iX;iFrame->bottom=r.iBr.iY;iFrame->sequence++;iMutex.Signal();}
private:
    CRedrawPublisher():iFrame(0){}RMutex iMutex;RChunk iChunk;TRedrawFrame* iFrame;
};
// Open and close per call: never keep the publisher's chunk alive after exit.
// Acknowledgement means DrawNow was issued for that region, not LCD presentation.
static TInt InvalidatePublishedRect(CCoeControl& bg,RWsSession& ws,TInt& area){
    area=0;RMutex mutex;TInt error=mutex.OpenGlobal(KRedrawMutex);if(error==KErrNotFound)return 0;if(error)return error;
    RChunk chunk;error=chunk.OpenGlobal(KRedrawChunk,EFalse);if(error){mutex.Close();return error==KErrNotFound?0:error;}
    if(chunk.Size()<TInt(sizeof(TRedrawFrame))){chunk.Close();mutex.Close();return KErrCorrupt;}
    if(mutex.Wait(1)!=KErrNone){chunk.Close();mutex.Close();return 0;}TRedrawFrame* frame=reinterpret_cast<TRedrawFrame*>(chunk.Base());TInt result=0;
    RChunk live;TCandidateShared* session=CandidateOpen(live);TBool own=session&&!session->stop&&session->record.state==ERunning&&session->record.owner==frame->owner&&session->record.nonceLo==frame->nonceLo&&session->record.nonceHi==frame->nonceHi;live.Close();
    if(!own)result=0;else if(frame->magic==0)result=0;else if(frame->magic!=KRedrawMagic)result=KErrCorrupt;
    else if(frame->sequence!=frame->acknowledged){
        TRect dirty(frame->left,frame->top,frame->right,frame->bottom);
        if(dirty.IsEmpty()||dirty.iTl.iX<0||dirty.iTl.iY<0||dirty.iBr.iX>360||dirty.iBr.iY>640||!bg.OwnsWindow())result=KErrCorrupt;
        else {bg.DrawNow(dirty);ws.Flush();frame->acknowledged=frame->sequence;area=dirty.Width()*dirty.Height();result=1;}
    }
    mutex.Signal();chunk.Close();mutex.Close();return result;
}
#endif
