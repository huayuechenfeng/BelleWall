#ifndef BELLEWALL_CANDIDATE_H
#define BELLEWALL_CANDIDATE_H
#include "sessionclock.h"
#include "consumerwatch.h"
#include "videotiming.h"
static void CandidateHelperL(const TDesC& action,const TCandidateRecord& record){
    TBuf<16> token;CandidateToken(record,token);TBuf<100> args(action);args.Append(' ');args.Append(token);
    RProcess p;User::LeaveIfError(p.Create(_L("C:\\sys\\bin\\bellerenderhost.exe"),args));CleanupClosePushL(p);TRequestStatus done;p.Logon(done);p.Resume();
    // Helper owns no wallpaper pixels; no desktop UI thread waits here.
    for(TInt n=0;done==KRequestPending&&n<200;n++)User::After(100000);
    if(done==KRequestPending){p.LogonCancel(done);User::WaitForRequest(done);User::Leave(KErrTimedOut);}User::LeaveIfError(p.ExitReason());CleanupStack::PopAndDestroy(&p);
}
static void CandidateStateL(TUint state){candidate->record.state=state;CandidateWriteL(fs,candidate->record);Status(_L("CANDIDATE state"),state);}
static void CandidateStopWorkerL(const TCandidateRecord& r){
    RChunk web;TInt opened=web.OpenGlobal(KWebChunk,EFalse);if(opened==KErrNotFound)return;User::LeaveIfError(opened);CleanupClosePushL(web);
    if(web.Size()<TInt(sizeof(TWebFrames))||web.Size()>1024*1024)User::Leave(KErrCorrupt);
    TWebFrames* f=reinterpret_cast<TWebFrames*>(web.Base());
    if(f->magic!=KWebMagic||TUint(f->owner)!=r.owner||f->nonceLo!=r.nonceLo||f->nonceHi!=r.nonceHi)User::Leave(KErrPermissionDenied);
    TUint pid=f->producer;f->stop=1;if(!pid){CleanupStack::PopAndDestroy(&web);return;}
    RProcess worker;TInt error=worker.Open(TProcessId(pid));if(error==KErrNone){CleanupClosePushL(worker);if(worker.SecureId().iId!=TInt(0xe7b31130))User::Leave(KErrPermissionDenied);
        TRequestStatus ended;worker.Logon(ended);for(TInt n=0;n<20&&ended==KRequestPending;n++)User::After(100000);
        if(ended==KRequestPending){worker.Kill(KErrTimedOut);User::WaitForRequest(ended);Log(_L("RECOVERY terminated matching web producer"));}CleanupStack::PopAndDestroy(&worker);
    }else if(error!=KErrNotFound)User::Leave(error);
    CleanupStack::PopAndDestroy(&web);
}
static void CandidateWaitForRestoreDesktopL(){
    RWsSession ws;User::LeaveIfError(ws.Connect());CleanupClosePushL(ws);CHWRMLight* light=CHWRMLight::NewLC();
    Log(_L("RECOVERY waiting for unlocked foreground desktop"));
    TInt stable=0;for(TInt n=0;n<120;n++){stable=DesktopReadyL(ws,*light)?stable+1:0;if(stable>=2)break;User::After(250000);}
    CleanupStack::PopAndDestroy(light);CleanupStack::PopAndDestroy(&ws);
    if(stable<2){Log(_L("RECOVERY desktop unavailable; journal retained"));User::Leave(KErrNotReady);}
    Log(_L("RECOVERY foreground desktop ready"));
}
static void CandidateRestoreL(TCandidateRecord& r){
    TCandidateRecord disk;CandidateReadL(fs,disk);if(!CandidateSame(r,disk))User::Leave(KErrPermissionDenied);r=disk;
    TEntry check;if(fs.Entry(KJournal,check)==KErrNone){TFileName original;TUint owner;TInt type;ReadJournalL(original,owner,type);}if(fs.Entry(KPagesJournal,check)==KErrNone)CandidateVerifyL(fs,KPagesJournal);
    r.state=ERecovering;CandidateWriteL(fs,r);
    CandidateStopWorkerL(r);
    // Detach closes the only project consumer before page/cache restoration.
    if(r.widgetIntent)CandidateHelperL(_L("--candidate-detach"),r);
    CandidateWaitForRestoreDesktopL();TRAPD(restored,RestoreL());
    // A menu/desktop transition may race the page activation acknowledgement.
    // Retry only a timeout, through the same checksum/conflict-checked journal.
    if(restored==KErrTimedOut){Log(_L("RECOVERY page activation timeout; retrying verified restore once"));CandidateWaitForRestoreDesktopL();TRAP(restored,RestoreL());}
    User::LeaveIfError(restored);TEntry e;if(fs.Entry(KJournal,e)!=KErrNotFound||fs.Entry(KPagesJournal,e)!=KErrNotFound)User::Leave(KErrInUse);
    User::LeaveIfError(fs.Delete(KCandidateJournal));Log(_L("CANDIDATE stopped; recovery transaction complete"));
}
static void CandidateBackupL(){
    TFileName original;CurrentL(original);TInt type=WallpaperTypeL();if(original.Length()||type!=0)User::Leave(KErrNotSupported);
    TFileName temp(KJournal);temp.Append(_L(".tmp"));RFile f;User::LeaveIfError(f.Replace(fs,temp,EFileWrite|EFileShareExclusive));CleanupClosePushL(f);const TUint magic=0x42574c33,owner=RProcess().Id().Id();User::LeaveIfError(f.Write(TPckgC<TUint>(magic)));User::LeaveIfError(f.Write(TPckgC<TUint>(owner)));User::LeaveIfError(f.Write(TPckgC<TInt>(type)));User::LeaveIfError(f.Write(TPckgC<TUint>(candidate->record.nonceLo)));User::LeaveIfError(f.Write(TPckgC<TUint>(candidate->record.nonceHi)));User::LeaveIfError(f.Flush());CleanupStack::PopAndDestroy(&f);CandidateSealL(fs,temp);User::LeaveIfError(fs.Rename(temp,KJournal));
}
static void CandidateHalt(TAny*){if(candidate){candidate->stop=1;candidate->record.state=EStopping;}}
static void CandidatePlayL(TInt kind){
    CandidateBackupL();candidate->record.widgetIntent=1;CandidateStateL(EBinding);CandidateHelperL(_L("--candidate-attach"),candidate->record);
    RWsSession ws;User::LeaveIfError(ws.Connect());CleanupClosePushL(ws);CHWRMLight* light=CHWRMLight::NewLC();
    for(TInt i=0;!DesktopReadyL(ws,*light);i++){if(candidate->stop||i>=120)User::Leave(KErrCancel);User::After(500000);}
    for(TInt check=0;candidate->verified==0&&check<50;check++)User::After(100000);if(candidate->verified!=1)User::Leave(KErrNotSupported);
    TFileName path;FrameL(830,path,0,360,640);BindPagesL(path);User::LeaveIfError(AknsWallpaperUtils::SetIdleWallpaper(path,0));User::After(1000000);
    User::LeaveIfError(RFbsSession::Connect());LiveCacheImage* image=LiveCacheImage::NewLC(path);image->EnableDirtyL();
    ContentFrames* video=0;CWebStreamReader* web=0;CFbsBitmap* bitmap=0;
    if(kind==1){video=ContentFrames::NewLC();}else if(kind==2){web=CWebStreamReader::NewLC();bitmap=new(ELeave)CFbsBitmap;CleanupStack::PushL(bitmap);User::LeaveIfError(bitmap->Create(TSize(180,320),EColor64K));}
    RFile metrics;User::LeaveIfError(metrics.Replace(fs,_L("C:\\data\\BelleWall\\candidate-metrics.csv"),EFileWrite|EFileShareAny));CleanupClosePushL(metrics);User::LeaveIfError(metrics.Write(_L8("wall_us,active_us,source,work_us,consumer\n")));
    const TBool continuous=candidate->record.seconds==0;SessionClock lifetime;lifetime.InitL();
    BenchClock clock;clock.InitL();TUint last=clock.Now();TInt64 elapsed=0,active=0,next=0;TInt source=-1,seq=0;TBool previousReady=EFalse,resumeTrace=EFalse,consumerWaiting=EFalse;TUint consumer=candidate->consumer;ConsumerWatch consumerWatch(consumer,0);TInt64 nextHealth=0,nextMetric=0,lastWebProgress=0;TUint contentCount=0;
    CandidateStateL(ERunning);CleanupStack::PushL(TCleanupItem(CandidateHalt,0));
    while((continuous||elapsed<TInt64(candidate->record.seconds)*1000000)&&!candidate->stop){
        const TInt64 delta=continuous?lifetime.StepUs():TInt64(clock.Us(last));last=clock.Now();elapsed+=delta;if(continuous&&delta>5000000){previousReady=EFalse;CandidateStateL(EPaused);if(web)web->Pause(ETrue);}else if(previousReady)active+=delta;candidate->heartbeat++;
        if(elapsed>=nextHealth){nextHealth=elapsed+(continuous?60000000:10000000);TInt bytes=0;TInt cells=User::AllocSize(bytes);TTimeIntervalMicroSeconds cpu;TInt cpuError=RThread().GetCpuTime(cpu);TBuf<200> health;health.Format(_L("HEALTH wall_us=%Ld active_us=%Ld heap_bytes=%d cells=%d thread_cpu_us=%Ld cpu_error=%d state=%u frames=%u"),elapsed,active,bytes,cells,cpu.Int64(),cpuError,candidate->record.state,contentCount);Log(health);}
        TFileName current;CurrentL(current);if(current.CompareF(path))User::Leave(KErrInUse);
        const TBool desktopReady=DesktopReadyL(ws,*light);consumer=candidate->consumer;const ConsumerWatch::Health health=consumerWatch.Sample(consumer,elapsed,desktopReady);
        if(health==ConsumerWatch::Failed){TBuf<320> detail;detail.Format(_L("CONSUMER timeout wall_us=%Ld active_us=%Ld gap_us=%Ld loop_delta_us=%Ld counter=%u state=%u stop=%d verified=%d focus_group=%d"),elapsed,active,TInt64(consumerWatch.Age(elapsed)),delta,consumer,candidate->record.state,candidate->stop,candidate->verified,ws.GetFocusWindowGroup());Log(detail);Log(_L("CANDIDATE consumer heartbeat timed out"));User::Leave(KErrDied);}
        if(health==ConsumerWatch::Waiting&&!consumerWaiting){Log(_L("CONSUMER waiting for fresh heartbeat; content paused"));consumerWaiting=ETrue;}
        else if(health==ConsumerWatch::Ready&&consumerWaiting){Log(desktopReady?_L("CONSUMER heartbeat resumed; reacquiring drawing resources"):_L("CONSUMER wait deferred while desktop is not foreground"));consumerWaiting=EFalse;}
        TBool ready=desktopReady&&health==ConsumerWatch::Ready;if(ready!=previousReady){
            if(ready){
                TRAPD(refresh,image->RefreshL(path));
                if(refresh==KErrNotSupported&&image->CompressedTarget()){
                    CandidateStateL(EPaused);if(web)web->Pause(ETrue);
                    RefreshCandidatePagesL(path);image->RefreshL(path);
                }else User::LeaveIfError(refresh);
                if(bitmap){bitmap->Reset();User::LeaveIfError(bitmap->Create(TSize(180,320),EColor64K));Log(_L("RESUME source bitmap recreated"));}
                source=-1;lastWebProgress=active;resumeTrace=ETrue;
                // Cache rebuilding is paused time, but still counts against the
                // wall-clock test budget. Force the first video frame to repaint.
                elapsed+=continuous?lifetime.StepUs():TInt64(clock.Us(last));last=clock.Now();
            }
            CandidateStateL(ready?ERunning:EPaused);if(web)web->Pause(!ready);previousReady=ready;
        }
        if(!ready){User::After(500000);continue;}
        if(active<next){User::After(TInt(Min(TInt64(video?33334:100000),next-active)));continue;}
        if(video)next=BelleVideoTiming::NextDueUs(active,video->fps,video->den);
        else next=(active/100000+1)*100000;
        TUint began=clock.Now();TBool produced=ETrue;
        if(video){TInt target=BelleVideoTiming::SourceFrame(active,video->fps,video->den,video->count);if(target==source)produced=EFalse;else {source=target;image->Blit(video->ReadL(source));}}
        else if(web){TInt render=0,copy=0;if(resumeTrace)Log(_L("RESUME before web buffer read"));produced=web->ReadL(*bitmap,seq,render,copy);if(!produced&&active-lastWebProgress>10000000){Log(_L("STREAM no frame progress for 10 active seconds"));User::Leave(KErrTimedOut);}if(produced){lastWebProgress=active;if(resumeTrace)Log(_L("RESUME before FBS blit"));source=seq;image->Blit(*bitmap);if(resumeTrace)Log(_L("RESUME FBS blit complete"));resumeTrace=EFalse;}}
        else {image->Paint(++source);}
        if(produced)contentCount++;
        if(produced&&(!continuous||elapsed>=nextMetric)){nextMetric=elapsed+10000000;TInt metricSize=0;User::LeaveIfError(metrics.Size(metricSize));if(metricSize>1024*1024){User::LeaveIfError(metrics.SetSize(0));TInt position=0;User::LeaveIfError(metrics.Seek(ESeekStart,position));User::LeaveIfError(metrics.Write(_L8("wall_us,active_us,source,work_us,consumer\n")));}TBuf8<160> row;row.Format(_L8("%Ld,%Ld,%d,%d,%u\n"),elapsed,active,source,clock.Us(began),consumer);User::LeaveIfError(metrics.Write(row));}
    }
    Log(_L("CANDIDATE loop completed; entering cleanup"));CandidateStateL(EStopping);CleanupStack::PopAndDestroy();if(web)web->Pause(ETrue);User::LeaveIfError(metrics.Flush());CleanupStack::PopAndDestroy(&metrics);
    if(bitmap)CleanupStack::PopAndDestroy(bitmap);if(web)CleanupStack::PopAndDestroy(web);if(video)CleanupStack::PopAndDestroy(video);
    CleanupStack::PopAndDestroy(image);RFbsSession::Disconnect();CleanupStack::PopAndDestroy(light);CleanupStack::PopAndDestroy(&ws);
}
static void CandidateGuardL(const TDesC& args){
    TCandidateRecord saved;CandidateReadL(fs,saved);if(!CandidateArgsMatch(saved,args))User::Leave(KErrPermissionDenied);
    RProcess p;User::LeaveIfError(p.Open(TProcessId(saved.owner)));CleanupClosePushL(p);if(p.SecureId().iId!=0xe7b31103)User::Leave(KErrPermissionDenied);
    RChunk live;TCandidateShared* same=CandidateOpen(live);TBool valid=same&&CandidateSame(saved,same->record);TUint initialBeat=same?same->heartbeat:0;live.Close();if(!valid)User::Leave(KErrPermissionDenied);SessionClock watchdog;watchdog.InitL();TRequestStatus ended;p.Logon(ended);RProcess::Rendezvous(KErrNone);
    TInt64 wall=0,stalled=0;TUint beat=initialBeat;
    while(ended==KRequestPending){
        User::After(1000000);if(ended!=KRequestPending)break;TInt64 delta=watchdog.StepUs();wall+=delta;
        RChunk probe;TCandidateShared* state=CandidateOpen(probe);TBool validState=state&&CandidateSame(saved,state->record);
        TUint phase=validState?state->record.state:EStopping;
        if(validState&&(state->heartbeat!=beat||delta>5000000)){beat=state->heartbeat;stalled=0;}else stalled+=delta;
        probe.Close();
        // The owner closes its shared chunk after verified restoration, just
        // before E32Main exits. Do not label that normal teardown a timeout.
        if(!validState){
            TEntry entry;const TBool restored=fs.Entry(KCandidateJournal,entry)==KErrNotFound&&fs.Entry(KJournal,entry)==KErrNotFound&&fs.Entry(KPagesJournal,entry)==KErrNotFound;
            if(restored){for(TInt n=0;n<20&&ended==KRequestPending;n++)User::After(100000);if(ended!=KRequestPending)break;}
        }
        const TInt64 grace=TInt64(phase<=EBinding?180:phase>=EStopping?120:30)*1000000;
        if(!validState||stalled>grace||(saved.seconds&&wall>TInt64(saved.seconds+180)*1000000)){Log(_L("CANDIDATE GUARD progress timeout; requesting stop"));break;}
    }
    if(ended==KRequestPending){RChunk chunk;TCandidateShared* s=CandidateOpen(chunk);if(s&&CandidateSame(saved,s->record))s->stop=1;chunk.Close();for(TInt n=0;n<80&&ended==KRequestPending;n++)User::After(250000);if(ended==KRequestPending){p.Kill(KErrTimedOut);User::WaitForRequest(ended);}}
    TBuf<160> outcome;TExitCategoryName category=p.ExitCategory();outcome.Format(_L("CANDIDATE GUARD owner exit_type=%d reason=%d category=%S"),p.ExitType(),p.ExitReason(),&category);Log(outcome);
    CleanupStack::PopAndDestroy(&p);
    RMutex lock;User::LeaveIfError(lock.CreateGlobal(_L("BelleWallWallpaperExperiment")));CleanupClosePushL(lock);TEntry e;if(fs.Entry(KCandidateJournal,e)==KErrNone){TCandidateRecord disk;CandidateReadL(fs,disk);if(CandidateSame(saved,disk))CandidateRestoreL(disk);}CleanupStack::PopAndDestroy(&lock);
}
static void CandidateCommandL(const TDesC& args){
    if(args.Find(_L("--candidate-guard"))==0){CandidateGuardL(args);return;}
    if(args==_L("--candidate-stop")){RChunk chunk;TCandidateShared* s=CandidateOpen(chunk);if(s)s->stop=1;chunk.Close();return;}
    RMutex lock;User::LeaveIfError(lock.CreateGlobal(_L("BelleWallWallpaperExperiment")));CleanupClosePushL(lock);
    TEntry e;if(fs.Entry(KCandidateJournal,e)!=KErrNotFound){TCandidateRecord old;CandidateReadL(fs,old);CandidateRestoreL(old);}
    if(fs.Entry(KJournal,e)!=KErrNotFound||fs.Entry(KPagesJournal,e)!=KErrNotFound)User::Leave(KErrInUse);
    if(args==_L("--candidate-recover")){CleanupStack::PopAndDestroy(&lock);return;}
    if(fs.Entry(_L("C:\\data\\BelleWall\\preparation-pending.bin"),e)!=KErrNotFound)User::Leave(KErrInUse);
    const TInt kind=args.Find(_L("--candidate-web"))==0?2:args.Find(_L("--candidate-video"))==0?1:args.Find(_L("--candidate-blocks"))==0?0:-1;if(kind<0)User::Leave(KErrArgument);
    RChunk chunk;User::LeaveIfError(chunk.CreateGlobal(KCandidateChunk,sizeof(TCandidateShared),sizeof(TCandidateShared)));CleanupClosePushL(chunk);candidate=reinterpret_cast<TCandidateShared*>(chunk.Base());Mem::FillZ(candidate,sizeof(*candidate));
    TTime now;now.UniversalTime();candidate->record.magic=0x31535742;candidate->record.version=BW_RENDER_SESSION_VERSION;candidate->record.nonceLo=TUint(now.Int64());candidate->record.nonceHi=TUint(now.Int64()>>32)^User::FastCounter();candidate->record.owner=RProcess().Id().Id();candidate->record.seconds=args.Find(_L("--continuous"))!=KErrNotFound?0:args.Find(_L("--600"))!=KErrNotFound?600:60;CandidateStateL(EPreparing);
    RProcess guard;TBuf<16> token;CandidateToken(candidate->record,token);TBuf<80> command(_L("--candidate-guard "));command.Append(token);User::LeaveIfError(guard.Create(RProcess().FileName(),command));CleanupClosePushL(guard);TRequestStatus ready;guard.Rendezvous(ready);RTimer timer;User::LeaveIfError(timer.CreateLocal());CleanupClosePushL(timer);TRequestStatus timeout;timer.After(timeout,5000000);guard.Resume();User::WaitForRequest(ready,timeout);if(ready==KRequestPending){guard.Kill(KErrTimedOut);User::WaitForRequest(ready);User::Leave(KErrTimedOut);}timer.Cancel();User::WaitForRequest(timeout);CleanupStack::PopAndDestroy(&timer);User::LeaveIfError(ready.Int());CleanupStack::PopAndDestroy(&guard);
    TRAPD(play,CandidatePlayL(kind));candidate->stop=1;candidate->record.state=EStopping;Status(_L("CANDIDATE playback result"),play);TRAPD(restore,CandidateRestoreL(candidate->record));Status(_L("CANDIDATE recovery result"),restore);candidate=0;CleanupStack::PopAndDestroy(&chunk);CleanupStack::PopAndDestroy(&lock);User::LeaveIfError(restore);User::LeaveIfError(play);
}
#endif
