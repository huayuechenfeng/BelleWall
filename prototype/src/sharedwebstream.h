#ifndef BELLEWALL_SHAREDWEBSTREAM_H
#define BELLEWALL_SHAREDWEBSTREAM_H
#include "webtransport.h"
class CWebStreamReader:public CBase{
public:
    static CWebStreamReader* NewLC(const TSize& size=TSize(180,320)){CWebStreamReader* self=new(ELeave)CWebStreamReader;CleanupStack::PushL(self);User::LeaveIfError(self->mutex.CreateGlobal(KWebMutex));User::LeaveIfError(self->chunk.CreateGlobal(KWebChunk,sizeof(TWebFrames),sizeof(TWebFrames)));self->frames=reinterpret_cast<TWebFrames*>(self->chunk.Base());Mem::FillZ(self->frames,sizeof(TWebFrames));self->frames->magic=KWebMagic;self->frames->owner=RProcess().Id().Id();self->frames->limitMs=40000;self->frames->requestedWidth=size.iWidth&~1;self->frames->requestedHeight=size.iHeight;RChunk live;TCandidateShared* session=CandidateOpen(live);TBuf<80> args(_L("--export-stream"));if(session){self->frames->nonceLo=session->record.nonceLo;self->frames->nonceHi=session->record.nonceHi;self->frames->limitMs=session->record.seconds?(session->record.seconds+120)*1000:0;TBuf<16> token;CandidateToken(session->record,token);args.Append(_L(" "));args.Append(token);}live.Close();User::LeaveIfError(self->process.Create(_L("C:\\sys\\bin\\belleweb.exe"),args));self->frames->producer=self->process.Id().Id();self->process.Resume();return self;}
    ~CWebStreamReader(){if(frames&&mutex.Wait(100000)==KErrNone){frames->stop=1;mutex.Signal();}if(process.Handle()){for(TInt i=0;i<20&&process.ExitType()==EExitPending;i++)User::After(100000);if(process.ExitType()==EExitPending&&process.SecureId().iId==TInt(0xe7b31130)){TRequestStatus ended;process.Logon(ended);process.Kill(KErrTimedOut);User::WaitForRequest(ended);Log(_L("STREAM terminated own unresponsive producer"));}else Status(_L("STREAM producer exit reason"),process.ExitReason());process.Close();}chunk.Close();mutex.Close();}
    TBool ReadL(CFbsBitmap& bitmap,TInt& seq,TInt& renderUs,TInt& copyUs){if(process.ExitType()!=EExitPending)User::Leave(KErrDied);User::LeaveIfError(mutex.Wait(100000));if(frames->sequence==seq||!frames->sequence){mutex.Signal();return EFalse;}if(frames->magic!=KWebMagic||frames->slot<0||frames->slot>1){mutex.Signal();User::Leave(KErrCorrupt);}if(!BelleDisplay::Valid(frames->width,frames->height)||(frames->width&1)||frames->generation!=frames->requestedGeneration){mutex.Signal();return EFalse;}if(bitmap.SizeInPixels()!=TSize(frames->width,frames->height)){bitmap.Reset();TInt error=bitmap.Create(TSize(frames->width,frames->height),EColor64K);if(error){mutex.Signal();User::Leave(error);}}bitmap.BeginDataAccess();Mem::Copy(bitmap.DataAddress(),frames->pixels[frames->slot],frames->width*frames->height*2);bitmap.EndDataAccess(EFalse);seq=frames->sequence;renderUs=frames->renderUs;copyUs=frames->copyUs;mutex.Signal();return ETrue;}
    void ResizeL(const TSize& size){if(!BelleDisplay::Valid(size.iWidth,size.iHeight))User::Leave(KErrNotSupported);User::LeaveIfError(mutex.Wait(100000));frames->requestedWidth=size.iWidth&~1;frames->requestedHeight=size.iHeight;frames->requestedGeneration++;frames->sequence=0;mutex.Signal();}
    void Pause(TBool pause){User::LeaveIfError(mutex.Wait(100000));frames->paused=pause;mutex.Signal();}
private:CWebStreamReader():frames(0){}RChunk chunk;RMutex mutex;RProcess process;TWebFrames* frames;
};
static void SharedWebStreamL(RWsSession& ws,CHWRMLight& light,LiveCacheImage& image,const TDesC& path,TBool localRedraw=EFalse){
    CWebStreamReader* reader=CWebStreamReader::NewLC();CFbsBitmap* bitmap=new(ELeave)CFbsBitmap;CleanupStack::PushL(bitmap);User::LeaveIfError(bitmap->Create(TSize(180,320),EColor64K));
    RBuf8 csv;csv.CreateL(64*1024);CleanupClosePushL(csv);csv.Append(_L8("index,sequence,render_us,producer_copy_us,read_us,blit_us,redraw_us,elapsed_us\n"));
    BenchClock clock;clock.InitL();TUint began=clock.Now(),start=began;TInt seq=0,count=0;TEntry entry;Log(_L("STREAM native latest-frame consumer started; no BMP/request files"));if(localRedraw)Log(_L("LOCAL BACKGROUND WebKit: no per-frame global redraw; 15 second content window"));
    while(clock.Us(began)<30000000&&(count==0||clock.Us(start)<(localRedraw?15000000:20000000))&&count<400){
        TFileName current;CurrentL(current);if(fs.Entry(KStop,entry)==KErrNone||current.CompareF(path))break;
        if(!DesktopReadyL(ws,light)){reader->Pause(ETrue);User::After(100000);continue;}reader->Pause(EFalse);
        TInt renderUs=0,copyUs=0;TUint read=clock.Now();if(!reader->ReadL(*bitmap,seq,renderUs,copyUs)){User::After(5000);continue;}TInt readUs=clock.Us(read);if(!count)start=clock.Now();
        TUint blit=clock.Now();image.Blit(*bitmap);TInt blitUs=clock.Us(blit);TUint redraw=clock.Now();if(!localRedraw){ws.ClearAllRedrawStores();User::LeaveIfError(ws.Finish());}TInt redrawUs=clock.Us(redraw);
        TBuf8<180> row;row.Format(_L8("%d,%d,%d,%d,%d,%d,%d,%d\n"),count,seq,renderUs,copyUs,readUs,blitUs,redrawUs,clock.Us(start));csv.Append(row);count++;
    }
    TBuf<120> line;line.Format(_L("STREAM native frames=%d elapsed_us=%d latest_sequence=%d"),count,clock.Us(start),seq);Log(line);
    RFile result;User::LeaveIfError(result.Replace(fs,_L("C:\\data\\BelleWall\\wallpaper-benchmark.csv"),EFileWrite|EFileShareExclusive));CleanupClosePushL(result);User::LeaveIfError(result.Write(csv));User::LeaveIfError(result.Flush());CleanupStack::PopAndDestroy(&result);
    CleanupStack::PopAndDestroy(&csv);CleanupStack::PopAndDestroy(bitmap);CleanupStack::PopAndDestroy(reader);
}
#endif
