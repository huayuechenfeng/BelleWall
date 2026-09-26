// Longrun renderer generation: must be packaged under its new ECom identity.
#include <e32base.h>
#include <f32file.h>
#include <e32property.h>
#include <avkondomainpskeys.h>
#include <hwrmlight.h>
#include <xnextrenderingpluginadapter.h>
#include <ecom/implementationproxy.h>
#include "renderlog.h"
#include "sessionclock.h"
static void Trace(const TDesC& event){BoundedRenderTrace(_L("C:\\data\\BelleWall\\render-longrun.log"),_L("C:\\data\\BelleWall\\render-longrun.previous.log"),event);}
#include "backgroundinspect.h"
#include "redrawtransport.h"
#include "candidatesession.h"
class CBelleRenderProbe:public CXnExtRenderingPluginAdapter{
public:
    static CBelleRenderProbe* NewL(){CBelleRenderProbe* self=new(ELeave)CBelleRenderProbe;CleanupStack::PushL(self);self->iClock.InitL();TBuf<100> line;line.Format(_L("PLUGIN longrun impl=e7b31137 construct pid=%u sid=%08x"),TUint(RProcess().Id().Id()),RProcess().SecureId().iId);Trace(line);CleanupStack::Pop(self);return self;}
    ~CBelleRenderProbe(){delete iTimer;delete iLight;TBuf<160> line;line.Format(_L("LOCAL ticks=%Ld draws=%Ld elapsed_ms=%Ld"),iTicks,iDraws,iElapsedUs/1000);Trace(line);line.Format(_L("DIRTY requests=%Ld partial=%Ld total_area=%Ld"),iRequests,iPartial,iArea);Trace(line);Trace(_L("PLUGIN destroyed"));}
    void SizeChanged(){TBuf<160> line;line.Format(_L("PLUGIN size=%dx%d window=%d"),Size().iWidth,Size().iHeight,Window().WsHandle());Trace(line);
        const CCoeControl* control=this;
        for(TInt depth=0;control&&depth<8;depth++,control=control->Parent()){
            const TRect r=control->Rect();line.Format(_L("CONTROL depth=%d own=%d rect=%d,%d,%d,%d ws=%d"),depth,control->OwnsWindow(),r.iTl.iX,r.iTl.iY,r.Width(),r.Height(),control->DrawableWindow()?control->DrawableWindow()->WsHandle():0);Trace(line);
        }
        if(!iInspected){iInspected=ETrue;TRAPD(inspectError,InspectBackgroundL());line.Format(_L("BACKGROUND inspect result=%d"),inspectError);Trace(line);}
        if(!iTimer){TRAPD(err,if(!iLight)iLight=CHWRMLight::NewL();iTimer=CPeriodic::NewL(CActive::EPriorityLow));if(err){line.Format(_L("LOCAL timer allocation error=%d"),err);Trace(line);return;}iTimer->Start(100000,100000,TCallBack(Tick,this));}
    }
    void EnterPowerSaveModeL(){iPaused=ETrue;Trace(_L("PLUGIN enter power save; widget inactive; background uses desktop/light/lock gate"));}
    void ExitPowerSaveModeL(){iPaused=EFalse;Trace(_L("PLUGIN exit power save"));}
private:
    static TInt Tick(TAny* ptr){
        CBelleRenderProbe* self=static_cast<CBelleRenderProbe*>(ptr);
        self->iElapsedUs+=self->iClock.StepUs();
        RChunk session;TCandidateShared* live=CandidateOpen(session);
        if(live&&!BelleRenderer::AcceptLongrun(live->record.version)){session.Close();self->iTimer->Cancel();Trace(_L("PLUGIN rejected other renderer session version"));return 0;}
        if(live){if(!self->iBound){self->iNonceLo=live->record.nonceLo;self->iNonceHi=live->record.nonceHi;self->iBound=ETrue;}else if(self->iNonceLo!=live->record.nonceLo||self->iNonceHi!=live->record.nonceHi){session.Close();self->iTimer->Cancel();return 0;}}
        const TInt period=live&&live->record.state==ERunning&&self->DesktopReady()?100000:500000;if(period!=self->iPeriod){self->iPeriod=period;self->iTimer->Cancel();self->iTimer->Start(period,period,TCallBack(Tick,self));}
        if(live&&!live->stop&&(live->record.state==EBinding||live->record.state==ERunning||live->record.state==EPaused)){
            if(live->record.state==EBinding){TRAPD(check,CCoeControl* bg=InspectBackgroundL(EFalse);if(!bg||bg->Rect()!=TRect(0,0,360,640))User::Leave(KErrNotSupported));live->verified=check?check:1;}
            live->consumer++;
            if(live->record.state==ERunning&&self->DesktopReady()){
                self->iTicks++;TRAPD(error,CCoeControl* bg=InspectBackgroundL(EFalse);if(bg&&bg->Rect()!=TRect(0,0,360,640))User::Leave(KErrNotSupported);if(bg&&bg->IsVisible()){TInt area=0;TInt requested=InvalidatePublishedRect(*bg,CCoeEnv::Static()->WsSession(),area);User::LeaveIfError(requested);if(requested){self->iRequests++;self->iArea+=area;if(area<360*640)self->iPartial++;}});
                if(error){self->iTimer->Cancel();Trace(_L("CANDIDATE background validation failed; consumer stopped"));}
            }
        }
        session.Close();return 1;
    }
    void Draw(const TRect&)const{iDraws++;}
    TBool DesktopReady()const{TInt lock=-1;if(!iLight||RProperty::Get(KPSUidAvkonDomain,KAknKeyguardStatus,lock)!=KErrNone||lock!=EKeyguardNotActive||iLight->LightStatus(CHWRMLight::EPrimaryDisplay)!=CHWRMLight::ELightOn)return EFalse;CCoeEnv* env=CCoeEnv::Static();return env->WsSession().GetFocusWindowGroup()==env->RootWin().Identifier();}
    CBelleRenderProbe():iNonceLo(0),iNonceHi(0),iBound(EFalse),iTimer(0),iPeriod(100000),iLight(0),iTicks(0),iDraws(0),iRequests(0),iPartial(0),iArea(0),iPaused(EFalse),iInspected(EFalse),iElapsedUs(0){}
    TUint iNonceLo,iNonceHi;TBool iBound;CPeriodic* iTimer;TInt iPeriod;CHWRMLight* iLight;TInt64 iTicks;mutable TInt64 iDraws;TInt64 iRequests,iPartial,iArea;TBool iPaused;TBool iInspected;SessionClock iClock;TInt64 iElapsedUs;
};
static const TImplementationProxy KImplementations[]={IMPLEMENTATION_PROXY_ENTRY(BW_RENDER_IMPL_UID,CBelleRenderProbe::NewL)};
EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& count){count=sizeof(KImplementations)/sizeof(KImplementations[0]);return KImplementations;}
