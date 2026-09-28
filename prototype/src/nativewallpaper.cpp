// Reversible, bounded experiment using the real system wallpaper API.
// Separate native guardian process restores the recorded wallpaper on exit/timeout.
#include <e32base.h>
#include <f32file.h>
#include <centralrepository.h>
#include <AknSkinsInternalCRKeys.h>
#include <aknswallpaperutils.h>
#include <w32std.h>
#include <apgwgnam.h>
#include <e32property.h>
#include <avkondomainpskeys.h>
#include <hwrmlight.h>
#include "candidatesession.h"
static TCandidateShared* candidate=0;
static void Log(const TDesC& message);
#include "nativevideo.h"
static RFs fs;
_LIT(KJournal,"C:\\data\\BelleWall\\wallpaper-rollback.bin");
_LIT(KStop,"C:\\data\\BelleWall\\native-stop");
_LIT(KOwnPrefix,"C:\\data\\BelleWall\\native-frame-");
static void Log(const TDesC& message){TEntry entry;if(fs.Entry(_L("C:\\data\\BelleWall\\native-wallpaper.log"),entry)==KErrNone&&entry.iSize>512*1024){fs.Delete(_L("C:\\data\\BelleWall\\native-wallpaper.previous.log"));if(fs.Rename(_L("C:\\data\\BelleWall\\native-wallpaper.log"),_L("C:\\data\\BelleWall\\native-wallpaper.previous.log")))return;}RFile f;if(f.Open(fs,_L("C:\\data\\BelleWall\\native-wallpaper.log"),EFileWrite|EFileShareAny)!=KErrNone)if(f.Create(fs,_L("C:\\data\\BelleWall\\native-wallpaper.log"),EFileWrite|EFileShareAny)!=KErrNone)return;TInt p=0;f.Seek(ESeekEnd,p);TBuf8<1024> line;line.Copy(message.Left(1000));line.Append(_L8("\r\n"));f.Write(line);f.Flush();f.Close();}
static void Status(const TDesC& label,TInt error){TBuf<256> line;line.Format(_L("%S = %d"),&label,error);Log(line);}
static void CurrentL(TDes& path){CRepository* c=CRepository::NewLC(KCRUidPersonalisation);User::LeaveIfError(c->Get(KPslnIdleBackgroundImagePath,path));CleanupStack::PopAndDestroy(c);}
static TInt WallpaperTypeL(){CRepository* c=CRepository::NewLC(KCRUidPersonalisation);TInt value=-1;User::LeaveIfError(c->Get(KPslnWallpaperType,value));CleanupStack::PopAndDestroy(c);return value;}
static void ReadJournalL(TDes& original,TUint& owner,TInt& type){
    TEntry candidateEntry;if(fs.Entry(KCandidateJournal,candidateEntry)==KErrNone){TCandidateRecord r;CandidateReadL(fs,r);CandidateVerifyL(fs,KJournal);RFile j;User::LeaveIfError(j.Open(fs,KJournal,EFileRead|EFileShareReadersOnly));CleanupClosePushL(j);TUint words[6];TPtr8 b(reinterpret_cast<TUint8*>(words),24,24);TInt size;User::LeaveIfError(j.Size(size));User::LeaveIfError(j.Read(b));if(size!=24||b.Length()!=24||words[0]!=0x42574c33||words[1]!=r.owner||words[2]!=0||words[3]!=r.nonceLo||words[4]!=r.nonceHi)User::Leave(KErrCorrupt);owner=words[1];type=0;original.Zero();CleanupStack::PopAndDestroy(&j);return;}
    RFile f;User::LeaveIfError(f.Open(fs,KJournal,EFileRead|EFileShareReadersOnly));CleanupClosePushL(f);
    TInt length;User::LeaveIfError(f.Size(length));if(length<12||length>12+2*original.MaxLength()||(length&1))User::Leave(KErrCorrupt);
    TPckgBuf<TUint> magic,pid;TPckgBuf<TInt> kind;User::LeaveIfError(f.Read(magic));User::LeaveIfError(f.Read(pid));User::LeaveIfError(f.Read(kind));if(magic()!=0x42574c32)User::Leave(KErrCorrupt);owner=pid();type=kind();
    TPtr8 bytes(reinterpret_cast<TUint8*>(const_cast<TUint16*>(original.Ptr())),0,original.MaxLength()*2);User::LeaveIfError(f.Read(bytes,length-12));if(bytes.Length()!=length-12)User::Leave(KErrCorrupt);original.SetLength(bytes.Length()/2);CleanupStack::PopAndDestroy(&f);
}
#include "hspages.h"
static TBool OwnWallpaperL(const TDesC& path){TEntry e;if(fs.Entry(KCandidateJournal,e)==KErrNone){TCandidateRecord r;CandidateReadL(fs,r);TBuf<16> token;CandidateToken(r,token);TFileName expected;expected.Format(_L("C:\\data\\BelleWall\\native-frame-%S-830.bmp"),&token);return path.CompareF(expected)==0;}return path.Left(KOwnPrefix().Length()).CompareF(KOwnPrefix)==0;}
static void RestoreL(){
    RestorePagesL();
    TEntry e;if(fs.Entry(KJournal,e)==KErrNotFound){Log(_L("RESTORE already complete"));return;}
    TFileName original,current;TUint owner;TInt type;ReadJournalL(original,owner,type);CurrentL(current);
    if(current.CompareF(original)!=0&&!OwnWallpaperL(current)){Log(_L("RESTORE skipped: user changed wallpaper; journal retained"));return;}
    TInt error=AknsWallpaperUtils::SetIdleWallpaper(original,0);Status(_L("RESTORE API"),error);User::LeaveIfError(error);
    CRepository* repository=CRepository::NewLC(KCRUidPersonalisation);User::LeaveIfError(repository->Set(KPslnWallpaperType,type));CleanupStack::PopAndDestroy(repository);
    CurrentL(current);if(current.CompareF(original)!=0)User::Leave(KErrCorrupt);
    if(WallpaperTypeL()!=type)User::Leave(KErrCorrupt);
    // Allow asynchronous desktop notifications to settle before discarding backup.
    for(TInt i=0;i<2;i++){User::After(1000000);CurrentL(current);if(current.CompareF(original)!=0||WallpaperTypeL()!=type)User::Leave(KErrNotReady);}
    User::LeaveIfError(fs.Delete(KJournal));Log(_L("RESTORE verified; journal removed"));
}
static void GuardL(TBool pages=EFalse){
    TFileName original;TUint owner;TInt type;ReadJournalL(original,owner,type);
    RProcess target;TInt opened=target.Open(TProcessId(owner));TRequestStatus ended;
    if(opened==KErrNone){if(target.SecureId().iId!=0xe7b31103){target.Close();User::Leave(KErrPermissionDenied);}target.Logon(ended);}
    RProcess::Rendezvous(KErrNone);Log(pages?_L("GUARD ready; maximum 90 seconds for four-page setup/playback/restore"):_L("GUARD ready; maximum 45 seconds"));
    if(opened==KErrNone){TBool completed=EFalse;for(TInt i=0;i<(pages?360:180)&&ended==KRequestPending;i++){TEntry e;if(fs.Entry(KJournal,e)==KErrNotFound){completed=ETrue;break;}User::After(250000);}if(ended==KRequestPending){if(!completed){Log(_L("GUARD timeout: stopping owner before rollback"));target.Kill(KErrTimedOut);}else target.LogonCancel(ended);User::WaitForRequest(ended);}if(target.ExitType()!=EExitPending){TBuf<120> outcome;TExitCategoryName category=target.ExitCategory();outcome.Format(_L("GUARD owner exit_type=%d reason=%d category=%S"),target.ExitType(),target.ExitReason(),&category);Log(outcome);}target.Close();}
    RestoreL();Log(_L("GUARD done"));
}
static void StartGuardL(TBool pages=EFalse){
    RProcess guard;User::LeaveIfError(guard.Create(_L("C:\\sys\\bin\\bellepaper.exe"),pages?_L("--guard-pages"):_L("--guard")));CleanupClosePushL(guard);
    TRequestStatus ready,timeout;guard.Rendezvous(ready);RTimer timer;User::LeaveIfError(timer.CreateLocal());CleanupClosePushL(timer);timer.After(timeout,5000000);guard.Resume();User::WaitForRequest(ready,timeout);
    if(ready==KRequestPending){guard.Kill(KErrTimedOut);User::WaitForRequest(ready);User::Leave(KErrTimedOut);}timer.Cancel();User::WaitForRequest(timeout);User::LeaveIfError(ready.Int());CleanupStack::PopAndDestroy(&timer);CleanupStack::PopAndDestroy(&guard);
}
// Explicit recovery of a project-owned test image after an asynchronous/page
// update reintroduces it. Refuses non-project paths and active experiments.
static void RecoverBenchmarkL(){
    RMutex mutex;User::LeaveIfError(mutex.CreateGlobal(_L("BelleWallWallpaperExperiment")));CleanupClosePushL(mutex);
    TFileName current;CurrentL(current);if(current.Length()&&current.Left(KOwnPrefix().Length()).CompareF(KOwnPrefix)!=0)User::Leave(KErrPermissionDenied);
    TEntry entry;if(fs.Entry(KJournal,entry)==KErrNotFound){RFile journal;User::LeaveIfError(journal.Create(fs,KJournal,EFileWrite|EFileShareExclusive));CleanupClosePushL(journal);TUint magic=0x42574c32,owner=0;TInt type=0;User::LeaveIfError(journal.Write(TPckgC<TUint>(magic)));User::LeaveIfError(journal.Write(TPckgC<TUint>(owner)));User::LeaveIfError(journal.Write(TPckgC<TInt>(type)));User::LeaveIfError(journal.Flush());CleanupStack::PopAndDestroy(&journal);}
    RestoreL();User::After(2000000);CurrentL(current);Log(_L("RECOVERY settled path:"));Log(current);Status(_L("RECOVERY settled type"),WallpaperTypeL());CleanupStack::PopAndDestroy(&mutex);
}
static void Put32(TDes8& b,TInt offset,TUint value){for(TInt i=0;i<4;i++)b[offset+i]=TUint8(value>>(8*i));}
static void FrameL(TInt frame,TDes& name,NativeVideoFrames* video=0,TInt frameWidth=180,TInt frameHeight=320,CFbsBitmap* captured=0){
    CFbsBitmap* bitmap=captured?captured:video?&video->FrameL(frame):0;
    const TInt width=bitmap?bitmap->SizeInPixels().iWidth:frameWidth,height=bitmap?bitmap->SizeInPixels().iHeight:frameHeight,stride=(width*3+3)&~3,total=54+stride*height;
    if(width<=0||height<=0||width>2048||height>2048)User::Leave(KErrTooBig);
    HBufC8* data=HBufC8::NewLC(total);TPtr8 b=data->Des();b.SetLength(total);b.FillZ();b[0]='B';b[1]='M';Put32(b,2,total);Put32(b,10,54);Put32(b,14,40);Put32(b,18,width);Put32(b,22,height);b[26]=1;b[28]=24;Put32(b,34,stride*height);
    const TBool benchPattern=frame>=100&&frame<200;const TInt blockWidth=benchPattern?Max(2,width/8):38;
    const TInt left=benchPattern?(frame%10)*(width-blockWidth)/7:(frame*11)%Max(1,width-38);
    if(bitmap){for(TInt y=0;y<height;y++){TPtr8 row(&b[54+y*stride],0,stride);bitmap->GetScanLine(row,TPoint(0,height-y-1),width,EColor16M);}}
    else for(TInt y=0;y<height;y++)for(TInt x=0;x<width;x++){const TInt at=54+y*stride+x*3;const bool green=x>=left&&x<left+blockWidth&&y>(benchPattern?height*2/5:130)&&y<(benchPattern?height*3/5:168);b[at]=frame==830?0:green?150:35;b[at+1]=frame==830?0:green?210:20;b[at+2]=frame==830?0:green?25:8;}
    if(candidate){TBuf<16> token;CandidateToken(candidate->record,token);name.Format(_L("C:\\data\\BelleWall\\native-frame-%S-%d.bmp"),&token,frame);}else name.Format(_L("C:\\data\\BelleWall\\native-frame-%d.bmp"),frame);
    RFile file;User::LeaveIfError(file.Replace(fs,name,EFileWrite|EFileShareExclusive));CleanupClosePushL(file);User::LeaveIfError(file.Write(b));User::LeaveIfError(file.Flush());CleanupStack::PopAndDestroy(&file);CleanupStack::PopAndDestroy(data);
}
class WebProducer:public CBase{
public:
    ~WebProducer(){if(process.Handle()){if(process.ExitType()==EExitPending){RFile stop;TInt err=stop.Replace(fs,_L("C:\\data\\BelleWall\\web-stop"),EFileWrite|EFileShareAny);if(!err)stop.Close();Status(_L("WEB cooperative stop marker"),err);for(TInt i=0;i<20&&process.ExitType()==EExitPending;i++)User::After(100000);if(process.ExitType()==EExitPending)Log(_L("WEB producer exit pending; own 60-second limit remains"));else Status(_L("WEB producer exit reason"),process.ExitReason());}process.Close();}}
    void StartL(TBool video){fs.Delete(_L("C:\\data\\BelleWall\\web-stop"));fs.Delete(_L("C:\\data\\BelleWall\\web-request"));fs.Delete(_L("C:\\data\\BelleWall\\web-ready"));TBuf<32> args;args.Copy(video?_L("--export-video"):_L("--export-web"));User::LeaveIfError(process.Create(_L("C:\\sys\\bin\\belleweb.exe"),args));process.Resume();}
    void FrameL(TInt frame,TDes& name){
        RFile request;TInt openError=KErrInUse;for(TInt retry=0;retry<20&&openError==KErrInUse;retry++){openError=request.Replace(fs,_L("C:\\data\\BelleWall\\web-request"),EFileWrite|EFileShareExclusive);if(openError==KErrInUse)User::After(10000);}User::LeaveIfError(openError);CleanupClosePushL(request);TBuf8<16> number;number.Num(frame);User::LeaveIfError(request.Write(number));User::LeaveIfError(request.Flush());CleanupStack::PopAndDestroy(&request);
        for(TInt i=0;i<100;i++){if(process.ExitType()!=EExitPending)User::Leave(KErrDied);RFile ack;if(ack.Open(fs,_L("C:\\data\\BelleWall\\web-ready"),EFileRead|EFileShareAny)==KErrNone){TBuf8<16> value;TInt error=ack.Read(value);ack.Close();if(!error&&value==number){name.Format(_L("C:\\data\\BelleWall\\native-frame-%d.bmp"),frame);return;}}User::After(100000);}User::Leave(KErrTimedOut);
    }
private:RProcess process;
};
static TBool DesktopReadyL(RWsSession& ws,CHWRMLight& light){
    TInt lock=-1;if(RProperty::Get(KPSUidAvkonDomain,KAknKeyguardStatus,lock)!=KErrNone||lock!=EKeyguardNotActive||light.LightStatus(CHWRMLight::EPrimaryDisplay)!=CHWRMLight::ELightOn)return EFalse;
    CApaWindowGroupName* fg=CApaWindowGroupName::NewLC(ws,ws.GetFocusWindowGroup());TUid uid=fg->AppUid();CleanupStack::PopAndDestroy(fg);return uid.iUid==0x102750f0;
}
static void ScreenProbeL(){
    User::LeaveIfError(RFbsSession::Connect());RWsSession ws;User::LeaveIfError(ws.Connect());CleanupClosePushL(ws);CHWRMLight* light=CHWRMLight::NewLC();if(!DesktopReadyL(ws,*light))User::Leave(KErrNotReady);
    CWsScreenDevice* screen=new(ELeave)CWsScreenDevice(ws);CleanupStack::PushL(screen);User::LeaveIfError(screen->Construct());CFbsBitmap* bitmap=new(ELeave)CFbsBitmap;CleanupStack::PushL(bitmap);User::LeaveIfError(bitmap->Create(screen->SizeInPixels(),EColor16M));User::LeaveIfError(screen->CopyScreenToBitmap(bitmap));TFileName name;FrameL(9000,name,0,180,320,bitmap);Log(_L("SCREEN probe saved"));CleanupStack::PopAndDestroy(bitmap);CleanupStack::PopAndDestroy(screen);CleanupStack::PopAndDestroy(light);CleanupStack::PopAndDestroy(&ws);RFbsSession::Disconnect();
}
#include "wallpaperbenchmark.h"
#include "sharedwallpaper.h"
#include "candidate.h"
static void ExperimentL(NativeVideoFrames* video=0,TInt producerMode=0,TBool crashTest=EFalse,TInt benchmark=0){
    RMutex mutex;User::LeaveIfError(mutex.CreateGlobal(_L("BelleWallWallpaperExperiment")));CleanupClosePushL(mutex);
    TEntry entry;if(fs.Entry(KPagesJournal,entry)!=KErrNotFound||fs.Entry(KJournal,entry)!=KErrNotFound){Log(_L("Pending recovery: restore before new experiment"));User::Leave(KErrInUse);}
    TFileName original;CurrentL(original);Log(_L("BACKUP current wallpaper:"));Log(original);
    // This first trial requires the user's selected native/default background.
    // A nonempty path may represent a per-page/custom configuration; do not alter it.
    const TInt type=WallpaperTypeL();Status(_L("BACKUP wallpaper type"),type);
    if(original.Length()||type!=0){Log(_L("REFUSE nondefault wallpaper configuration for first experiment"));User::Leave(KErrNotSupported);}
    RFile journal;User::LeaveIfError(journal.Create(fs,KJournal,EFileWrite|EFileShareExclusive));CleanupClosePushL(journal);
    const TUint magic=0x42574c32,owner=RProcess().Id().Id();User::LeaveIfError(journal.Write(TPckgC<TUint>(magic)));User::LeaveIfError(journal.Write(TPckgC<TUint>(owner)));User::LeaveIfError(journal.Write(TPckgC<TInt>(type)));User::LeaveIfError(journal.Write(TPtrC8(reinterpret_cast<const TUint8*>(original.Ptr()),original.Size())));User::LeaveIfError(journal.Flush());CleanupStack::PopAndDestroy(&journal);
    StartGuardL(benchmark==13||benchmark==17);fs.Delete(KStop);
    WebProducer* producer=0;if(producerMode){producer=new(ELeave)WebProducer;CleanupStack::PushL(producer);producer->StartL(producerMode==2);}
    RWsSession ws;User::LeaveIfError(ws.Connect());CleanupClosePushL(ws);CHWRMLight* light=CHWRMLight::NewLC();
    TInt frame=0;if(benchmark>=6)SharedWallpaperL(ws,*light,original,benchmark);else if(benchmark)BenchmarkL(ws,*light,original,benchmark);
    for(TInt i=0;!benchmark&&i<100&&frame<20;i++){
        if(fs.Entry(KStop,entry)==KErrNone)break;
        if(DesktopReadyL(ws,*light)){
            TFileName current;CurrentL(current);if(current.CompareF(original)!=0&&current.Left(KOwnPrefix().Length()).CompareF(KOwnPrefix)!=0){Log(_L("User changed wallpaper; stopping"));break;}
            TFileName path;if(producer)producer->FrameL(frame,path);else FrameL(frame,path,video);
            // Rendering may take a second; recheck focus, light and lock before applying.
            if(!DesktopReadyL(ws,*light))continue;
            CurrentL(current);if(current.CompareF(original)!=0&&current.Left(KOwnPrefix().Length()).CompareF(KOwnPrefix)!=0)break;
            TInt result=AknsWallpaperUtils::SetIdleWallpaper(path,0);Status(_L("FRAME wallpaper API"),result);User::LeaveIfError(result);++frame;
            if(crashTest&&frame==3){Log(_L("TEST deliberate owner panic after third frame"));User::Panic(_L("BelleWallRecovery"),KErrAbort);}
            User::After(750000);
        }
        User::After(250000);
    }
    CleanupStack::PopAndDestroy(light);CleanupStack::PopAndDestroy(&ws);if(producer)CleanupStack::PopAndDestroy(producer);RestoreL();CleanupStack::PopAndDestroy(&mutex);
}
static void RenderingLibraryProbeL(){
    RLibrary library;TInt error=library.Load(_L("Z:\\sys\\bin\\extrenderingplugin.dll"));
    Status(_L("RENDER LIB load"),error);User::LeaveIfError(error);CleanupClosePushL(library);
    Log(_L("RENDER LIB path:"));Log(library.FileName());
    TInt exports=0;for(TInt ordinal=1;ordinal<=64;ordinal++)if(library.Lookup(ordinal))exports++;
    Status(_L("RENDER LIB present exports in 1..64"),exports);
    Log(_L("RENDER LIB only loaded and inspected; no plugin instantiated or registered"));
    CleanupStack::PopAndDestroy(&library);
}
GLDEF_C TInt E32Main(){
    CTrapCleanup* cleanup=CTrapCleanup::New();if(!cleanup)return KErrNoMemory;
    TInt result=fs.Connect();if(result){delete cleanup;return result;}fs.MkDirAll(_L("C:\\data\\BelleWall\\"));
    CActiveScheduler* scheduler=new CActiveScheduler;if(!scheduler){fs.Close();delete cleanup;return KErrNoMemory;}CActiveScheduler::Install(scheduler);
    TBuf<128> args;User::CommandLine(args);
    TRAP(result,
        TEntry liveJournal;if(args.Find(_L("--candidate"))!=0&&fs.Entry(KCandidateJournal,liveJournal)!=KErrNotFound)User::Leave(KErrInUse);
        if(args.Find(_L("--candidate"))!=KErrNotFound)CandidateCommandL(args);
        else if(args.Find(_L("--guard"))!=KErrNotFound)GuardL(args.Find(_L("--guard-pages"))!=KErrNotFound);
        else if(args.Find(_L("--page-activate"))!=KErrNotFound){TBuf8<32> before;TBuf8<32> after;PageActiveL(before);CHspsWrapper* hs=PagesClientLC();PageActivateL(*hs,_L8("1"));CleanupStack::PopAndDestroy(hs);PageActiveL(after);TBuf<80> line;TBuf<80> b;TBuf<80> a;b.Copy(before);a.Copy(after);line.Format(_L("CONTENT ACTIVATE before=%S after=%S"),&b,&a);Log(line);}
        else if(args.Find(_L("--restore"))!=KErrNotFound){RMutex recovery;User::LeaveIfError(recovery.CreateGlobal(_L("BelleWallWallpaperExperiment")));CleanupClosePushL(recovery);RestoreL();CleanupStack::PopAndDestroy(&recovery);}
        else if(args.Find(_L("--recover-benchmark"))!=KErrNotFound)RecoverBenchmarkL();
        else if(args.Find(_L("--render-plugin-probe"))!=KErrNotFound)RenderingLibraryProbeL();
        else if(args.Find(_L("--wallpaper-state"))!=KErrNotFound){TFileName path;CurrentL(path);Log(_L("STATE wallpaper path:"));Log(path);Status(_L("STATE wallpaper type"),WallpaperTypeL());}
        else if(args.Find(_L("--cache-live"))!=KErrNotFound)ExperimentL(0,0,EFalse,args.Find(_L("--cache-live-local-pages"))!=KErrNotFound?17:args.Find(_L("--cache-live-local-web"))!=KErrNotFound?16:args.Find(_L("--cache-live-local-content"))!=KErrNotFound?15:args.Find(_L("--cache-live-local"))!=KErrNotFound?14:args.Find(_L("--cache-live-pages"))!=KErrNotFound?13:args.Find(_L("--cache-live-stream"))!=KErrNotFound?12:args.Find(_L("--cache-live-web"))!=KErrNotFound?11:args.Find(_L("--cache-live-content"))!=KErrNotFound?10:args.Find(_L("--cache-live-flush"))!=KErrNotFound?9:args.Find(_L("--cache-live-speed"))!=KErrNotFound?8:args.Find(_L("--cache-live-redraw"))!=KErrNotFound?7:6);
        else if(args.Find(_L("--screen-probe"))!=KErrNotFound)ScreenProbeL();
        else if(args.Find(_L("--benchmark"))!=KErrNotFound)ExperimentL(0,0,EFalse,args.Find(_L("--benchmark-observe"))!=KErrNotFound?5:args.Find(_L("--benchmark-limit"))!=KErrNotFound?4:args.Find(_L("--benchmark-size"))!=KErrNotFound?3:args.Find(_L("--benchmark-fast"))!=KErrNotFound?2:1);
        else if(args.Find(_L("--software-video"))!=KErrNotFound)ExperimentL(0,2);
        else if(args.Find(_L("video"))!=KErrNotFound){
            User::LeaveIfError(RFbsSession::Connect());NativeVideoFrames* video=NativeVideoFrames::NewL();CleanupStack::PushL(video);Log(_L("VIDEO opening local sample.mp4; frame extraction, no audio playback"));video->OpenL();Log(_L("VIDEO prepared"));
            if(args.Find(_L("probe-video"))!=KErrNotFound){TFileName name;FrameL(0,name,video);Log(_L("VIDEO first frame extracted"));}
            else ExperimentL(video);
            CleanupStack::PopAndDestroy(video);RFbsSession::Disconnect();
        } else ExperimentL(0,args.Find(_L("--web"))!=KErrNotFound,args.Find(_L("--crash-test"))!=KErrNotFound);
    );
    // A stop command is only an acknowledgement; the session owner records the
    // actual playback/restoration result. Journals remain the recovery authority.
    if(args.Find(_L("--candidate"))==0&&args.Find(_L("--candidate-stop"))!=0&&args.Find(_L("--candidate-guard"))!=0){
        _LIT(KResult,"C:\\data\\BelleWall\\candidate-last-result.txt");_LIT(KResultTemp,"C:\\data\\BelleWall\\candidate-last-result.tmp");
        if(result==KErrNone)fs.Delete(KResult);
        else {RFile outcome;if(outcome.Replace(fs,KResultTemp,EFileWrite|EFileShareExclusive)==KErrNone){TBuf8<24> value;value.Num(result);TInt written=outcome.Write(value);if(!written)written=outcome.Flush();outcome.Close();if(!written)fs.Replace(KResultTemp,KResult);else fs.Delete(KResultTemp);}}
    }
    Status(_L("EXIT"),result);delete scheduler;fs.Close();delete cleanup;return result;
}
