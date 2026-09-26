// Standalone cache-sharing experiment. Never selects or changes desktop wallpaper.
// Only a dedicated project image is decoded; one cached pixel is changed/restored.
#include <e32base.h>
#include <f32file.h>
#include <fbs.h>
#include <bitdev.h>
#include <bitstd.h>
#include <AknsSrvClient.h>
static RFs fs;
_LIT(KImage,"C:\\data\\BelleWall\\cache-probe-only.bmp");
static void Log(const TDesC& text){RFile f;if(f.Open(fs,_L("C:\\data\\BelleWall\\cache-probe.log"),EFileWrite|EFileShareAny)!=KErrNone)User::LeaveIfError(f.Create(fs,_L("C:\\data\\BelleWall\\cache-probe.log"),EFileWrite|EFileShareAny));TInt pos=0;f.Seek(ESeekEnd,pos);TBuf8<512> line;line.Copy(text.Left(500));line.Append(_L8("\r\n"));f.Write(line);f.Flush();f.Close();}
static void Put32(TDes8& b,TInt p,TUint n){for(TInt i=0;i<4;i++)b[p+i]=TUint8(n>>(8*i));}
class CacheSession:public RAknsSrvSession{
public:void FetchL(CFbsBitmap*& image){TPckgBuf<TInt> handle,mask;handle()=0;mask()=0;const TSize target(360,640);TPckgC<TSize> size(target);Log(_L("CACHE raw IPC 21 begin"));TInt result=SendReceive(21,TIpcArgs(&KImage,&size,&handle,&mask));TBuf<128> line;line.Format(_L("CACHE raw IPC result=%d bitmap=%d mask=%d"),result,handle(),mask());Log(line);User::LeaveIfError(result);if(!handle()||mask())User::Leave(KErrNotSupported);image=new(ELeave)CFbsBitmap;User::LeaveIfError(image->Duplicate(handle()));Log(_L("CACHE duplicate ready"));}
};
static void PeerReadL(){CacheSession session;User::LeaveIfError(session.Connect());CleanupClosePushL(session);CFbsBitmap* image=0;session.FetchL(image);CleanupStack::PushL(image);TRgb pixel;image->GetPixel(pixel,TPoint(4,4));TBuf<160> line;line.Format(_L("CACHE PEER pid=%u handle=%d rgb=%d,%d,%d"),TUint(RProcess().Id().Id()),image->Handle(),pixel.Red(),pixel.Green(),pixel.Blue());Log(line);const TBool match=pixel.Red()==0&&pixel.Green()==255&&pixel.Blue()==0;CleanupStack::PopAndDestroy(image);CleanupStack::PopAndDestroy(&session);if(!match)User::Leave(KErrCorrupt);}
static TInt CheckPeerL(){RProcess peer;User::LeaveIfError(peer.Create(_L("C:\\sys\\bin\\bellecache.exe"),_L("--peer")));CleanupClosePushL(peer);TRequestStatus ended,timeout;peer.Logon(ended);RTimer timer;User::LeaveIfError(timer.CreateLocal());CleanupClosePushL(timer);timer.After(timeout,5000000);peer.Resume();User::WaitForRequest(ended,timeout);if(ended==KRequestPending){peer.Kill(KErrTimedOut);User::WaitForRequest(ended);}timer.Cancel();User::WaitForRequest(timeout);TInt result=peer.ExitType()==EExitKill?peer.ExitReason():KErrDied;TBuf<100> line;line.Format(_L("CACHE PEER result=%d exit_type=%d"),result,peer.ExitType());Log(line);CleanupStack::PopAndDestroy(&timer);CleanupStack::PopAndDestroy(&peer);return result;}
static void SuperviseL(){
    RProcess worker;User::LeaveIfError(worker.Create(_L("C:\\sys\\bin\\bellecache.exe"),_L("--worker")));CleanupClosePushL(worker);TRequestStatus ended,timeout;worker.Logon(ended);RTimer timer;User::LeaveIfError(timer.CreateLocal());CleanupClosePushL(timer);timer.After(timeout,15000000);worker.Resume();User::WaitForRequest(ended,timeout);if(ended==KRequestPending){worker.Kill(KErrTimedOut);User::WaitForRequest(ended);}timer.Cancel();User::WaitForRequest(timeout);TExitCategoryName category=worker.ExitCategory();TBuf<160> line;line.Format(_L("CACHE WORKER exit_type=%d reason=%d category=%S"),worker.ExitType(),worker.ExitReason(),&category);Log(line);CleanupStack::PopAndDestroy(&timer);CleanupStack::PopAndDestroy(&worker);
}
static void MakeImageL(){const TInt width=180,height=320,stride=width*3,total=54+stride*height;HBufC8* bytes=HBufC8::NewLC(total);TPtr8 b=bytes->Des();b.SetLength(total);b.FillZ();b[0]='B';b[1]='M';Put32(b,2,total);Put32(b,10,54);Put32(b,14,40);Put32(b,18,width);Put32(b,22,height);b[26]=1;b[28]=24;Put32(b,34,stride*height);for(TInt i=54;i<total;i+=3){b[i]=35;b[i+1]=20;b[i+2]=8;}RFile file;User::LeaveIfError(file.Replace(fs,KImage,EFileWrite|EFileShareExclusive));CleanupClosePushL(file);User::LeaveIfError(file.Write(b));User::LeaveIfError(file.Flush());CleanupStack::PopAndDestroy(&file);CleanupStack::PopAndDestroy(bytes);}
class Probe:public CBase{
public:
    Probe():a(0),b(0),am(0),bm(0),device(0),gc(0),opened(EFalse){}
    ~Probe(){delete gc;delete device;delete a;delete b;delete am;delete bm;if(opened){session.RemoveWallpaper(KImage);session.Close();}}
    void RunL(){
        MakeImageL();User::LeaveIfError(session.Connect());opened=ETrue;
        Log(_L("CACHE begin: dedicated image only; desktop is untouched"));
        session.FetchL(a);
        session.FetchL(b);
        if(!a||!b)User::Leave(KErrNotFound);
        TBuf<256> line;TSize size=a->SizeInPixels();line.Format(_L("CACHE a=%d b=%d width=%d height=%d mode=%d compressed=%d"),a->Handle(),b->Handle(),size.iWidth,size.iHeight,a->DisplayMode(),a->IsCompressedInRAM());Log(line);
        if(size.iWidth<8||size.iHeight<8||size.iWidth>640||size.iHeight>640||b->SizeInPixels()!=size)User::Leave(KErrNotSupported);
        TRgb beforeA,beforeB,afterA,afterB;const TPoint point(4,4);a->GetPixel(beforeA,point);b->GetPixel(beforeB,point);
        device=CFbsBitmapDevice::NewL(a);User::LeaveIfError(device->CreateContext(gc));gc->SetPenStyle(CGraphicsContext::ENullPen);gc->SetBrushStyle(CGraphicsContext::ESolidBrush);gc->SetBrushColor(TRgb(0,255,0));gc->DrawRect(TRect(point,TSize(1,1)));
        a->GetPixel(afterA,point);b->GetPixel(afterB,point);
        line.Format(_L("CACHE same_handle=%d writer_changed=%d peer_changed=%d peer_matches_writer=%d"),a->Handle()==b->Handle(),afterA.Value()!=beforeA.Value(),afterB.Value()!=beforeB.Value(),afterA.Value()==afterB.Value());Log(line);
        const TInt peerError=CheckPeerL();
        gc->SetBrushColor(beforeA);gc->DrawRect(TRect(point,TSize(1,1)));a->GetPixel(afterA,point);b->GetPixel(afterB,point);
        if(afterA.Value()!=beforeA.Value()||afterB.Value()!=beforeB.Value())User::Leave(KErrCorrupt);
        Log(_L("CACHE pixel restoration verified; releasing own cache entry"));
        User::LeaveIfError(peerError);
    }
private:CacheSession session;CFbsBitmap *a,*b,*am,*bm;CFbsBitmapDevice* device;CFbsBitGc* gc;TBool opened;
};
GLDEF_C TInt E32Main(){CTrapCleanup* cleanup=CTrapCleanup::New();if(!cleanup)return KErrNoMemory;TInt error=fs.Connect();if(error){delete cleanup;return error;}fs.MkDirAll(_L("C:\\data\\BelleWall\\"));CActiveScheduler* scheduler=new CActiveScheduler;if(!scheduler){fs.Close();delete cleanup;return KErrNoMemory;}CActiveScheduler::Install(scheduler);TBuf<64> args;User::CommandLine(args);if(args.Find(_L("--worker"))==KErrNotFound&&args.Find(_L("--peer"))==KErrNotFound){TRAP(error,SuperviseL());}else{error=RFbsSession::Connect();if(!error){if(args.Find(_L("--peer"))!=KErrNotFound){TRAP(error,PeerReadL());}else{TRAP(error,Probe* p=new(ELeave)Probe;CleanupStack::PushL(p);p->RunL();CleanupStack::PopAndDestroy(p));}RFbsSession::Disconnect();}}TBuf<80> line;line.Format(_L("CACHE EXIT=%d"),error);TRAP_IGNORE(Log(line));delete scheduler;fs.Close();delete cleanup;return error;}
