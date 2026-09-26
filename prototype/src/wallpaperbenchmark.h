#ifndef BELLEWALL_WALLPAPERBENCHMARK_H
#define BELLEWALL_WALLPAPERBENCHMARK_H
#include <hal.h>
class BenchClock {
public:
    void InitL(){User::LeaveIfError(HAL::Get(HALData::EFastCounterFrequency,hz));User::LeaveIfError(HAL::Get(HALData::EFastCounterCountsUp,up));if(hz<=0)User::Leave(KErrNotSupported);}
    TUint Now()const{return User::FastCounter();}
    TInt Us(TUint start)const{return TInt(TInt64(up?TUint(Now()-start):TUint(start-Now()))*1000000/hz);}
    TInt hz,up;
};
// Measures serial API throughput, not physical display presentation timestamps.
// Prebuilt files isolate wallpaper-service cost from image generation and writes.
static TBool BenchGate(RWsSession& ws,CHWRMLight& light,TInt desktopGroup){
    TInt lock=-1;return RProperty::Get(KPSUidAvkonDomain,KAknKeyguardStatus,lock)==KErrNone&&lock==EKeyguardNotActive&&light.LightStatus(CHWRMLight::EPrimaryDisplay)==CHWRMLight::ELightOn&&ws.GetFocusWindowGroup()==desktopGroup;
}
class BenchmarkScreen:public CBase{
public:
    static BenchmarkScreen* NewLC(RWsSession& ws){BenchmarkScreen* self=new(ELeave)BenchmarkScreen;CleanupStack::PushL(self);User::LeaveIfError(RFbsSession::Connect());self->connected=ETrue;self->screen=new(ELeave)CWsScreenDevice(ws);User::LeaveIfError(self->screen->Construct());self->bitmap=new(ELeave)CFbsBitmap;User::LeaveIfError(self->bitmap->Create(self->screen->SizeInPixels(),EColor16M));return self;}
    ~BenchmarkScreen(){delete bitmap;delete screen;if(connected)RFbsSession::Disconnect();}
    TInt SampleL(TInt& count){User::LeaveIfError(screen->CopyScreenToBitmap(bitmap));TInt sum=0;count=0;TSize size=bitmap->SizeInPixels();
        // Only record marker centroid, never icon titles or other screen data.
        for(TInt y=100;y<Min(390,size.iHeight);y+=3)for(TInt x=0;x<size.iWidth;x+=3){TRgb c;bitmap->GetPixel(c,TPoint(x,y));if(c.Green()>150&&c.Red()<70&&c.Blue()>90&&c.Blue()<190){sum+=x;++count;}}
        return count?sum/count:-1;
    }
    void SaveL(){TFileName name;FrameL(9000,name,0,180,320,bitmap);}
private:BenchmarkScreen():screen(0),bitmap(0),connected(EFalse){}CWsScreenDevice* screen;CFbsBitmap* bitmap;TBool connected;
};
static void BenchmarkL(RWsSession& ws,CHWRMLight& light,const TDesC& original,TInt mode){
    const TBool fast=mode>=2,sizes=mode>=3,lean=mode>=4,observe=mode==5;
    BenchClock clock;clock.InitL();TUint began=clock.Now();TEntry entry;
    TBuf<32> oldA,oldB;oldA.Format(_L("%02d"),100);oldB.Format(_L("%02d"),107);TBuf<128> diagnostic;diagnostic.Format(_L("BENCH legacy filename fields: 100=[%S] 107=[%S]"),&oldA,&oldB);Log(diagnostic);
    RBuf8 csv;csv.CreateL(128*1024);CleanupClosePushL(csv);
    csv.Append(_L8("phase,target_us,index,prepare_us,check_us,api_us,cycle_us,result,phase_elapsed_us,repo_us,gate_us,outer_us,sample_us,marker_x,marker_count\n"));
    TBuf<160> message;message.Format(_L("BENCH begin pid=%u counter_hz=%d up=%d mode=%d"),TUint(RProcess().Id().Id()),clock.hz,clock.up,mode);Log(message);
    const TUint preparing=clock.Now();if(!sizes)for(TInt n=0;n<20;n++){TFileName name;FrameL(n,name);}
    if(!sizes){message.Format(_L("BENCH prebuilt 20 BMP files in %d us"),clock.Us(preparing));Log(message);}
    while(!DesktopReadyL(ws,light)){if(clock.Us(began)>12000000||fs.Entry(KStop,entry)==KErrNone)User::Leave(KErrCancel);User::After(100000);}
    const TInt desktopGroup=ws.GetFocusWindowGroup();CRepository* repository=CRepository::NewLC(KCRUidPersonalisation);
    BenchmarkScreen* observer=observe?BenchmarkScreen::NewLC(ws):0;
    TInt targets[]={1000000,500000,200000,100000,0};if(fast){targets[0]=500000;targets[1]=200000;targets[2]=100000;targets[3]=33333;}TBool stop=EFalse;TInt serial=0;
    const TInt leanWidths[]={180,90,45,90},leanHeights[]={320,160,80,160};
    const TInt limitWidths[]={360,180,90,45},limitHeights[]={640,320,160,80};
    for(TInt phase=0;phase<(lean?4:sizes?3:5)&&!stop;phase++){
        if(sizes){TInt width=mode==4?limitWidths[phase]:lean?leanWidths[phase]:90<<phase,height=mode==4?limitHeights[phase]:lean?leanHeights[phase]:160<<phase;TUint gen=clock.Now();for(TInt n=0;n<8;n++){TFileName name;FrameL(100+phase*10+n,name,0,width,height);}message.Format(_L("BENCH size phase=%d width=%d height=%d prep_us=%d"),phase,width,height,clock.Us(gen));Log(message);targets[phase]=0;}
        const TUint phaseStart=clock.Now();TInt count=0;
        while(clock.Us(phaseStart)<(lean?5000000:sizes?6000000:4000000)&&count<256){
            TUint outer=clock.Now();
            if(clock.Us(began)>35000000||fs.Entry(KStop,entry)==KErrNone||(!lean&&((sizes||(fast&&phase>=2))?!BenchGate(ws,light,desktopGroup):!DesktopReadyL(ws,light)))){stop=ETrue;break;}
            const TInt outerUs=clock.Us(outer);
            TUint cycle=clock.Now(),prepare=cycle;TFileName path;
            if(phase==0&&!fast)FrameL(serial%20,path);else path.Format(_L("C:\\data\\BelleWall\\native-frame-%d.bmp"),sizes?100+phase*10+serial%8:serial%20);
            const TInt prepareUs=clock.Us(prepare);TUint check=clock.Now();TFileName current;
            if(sizes||(fast&&phase>=1))User::LeaveIfError(repository->Get(KPslnIdleBackgroundImagePath,current));else CurrentL(current);
            const TInt repoUs=clock.Us(check);TUint gate=clock.Now();
            if((current.CompareF(original)!=0&&current.Left(KOwnPrefix().Length()).CompareF(KOwnPrefix)!=0)||((sizes||(fast&&phase>=2))?!BenchGate(ws,light,desktopGroup):!DesktopReadyL(ws,light))){stop=ETrue;break;}
            const TInt gateUs=clock.Us(gate);
            const TInt checkUs=clock.Us(check);TUint api=clock.Now();TInt error=AknsWallpaperUtils::SetIdleWallpaper(path,0);const TInt apiUs=clock.Us(api);
            TInt markerX=-1,markerCount=0,sampleUs=0;if(observer&&!error){TUint sample=clock.Now();markerX=observer->SampleL(markerCount);sampleUs=clock.Us(sample);}
            const TInt remaining=targets[phase]-clock.Us(cycle);if(remaining>0)User::After(remaining);
            const TInt cycleUs=clock.Us(cycle);TBuf8<256> row;row.Format(_L8("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n"),phase,targets[phase],count,prepareUs,checkUs,apiUs,cycleUs,error,clock.Us(phaseStart),repoUs,gateUs,outerUs,sampleUs,markerX,markerCount);
            if(csv.MaxLength()-csv.Length()<row.Length()){stop=ETrue;break;}csv.Append(row);++count;++serial;
            if(error){stop=ETrue;break;}
        }
        message.Format(_L("BENCH phase=%d target_us=%d count=%d elapsed_us=%d"),phase,targets[phase],count,clock.Us(phaseStart));Log(message);
    }
    if(observer){observer->SaveL();CleanupStack::PopAndDestroy(observer);}CleanupStack::PopAndDestroy(repository);
    RFile result;User::LeaveIfError(result.Replace(fs,_L("C:\\data\\BelleWall\\wallpaper-benchmark.csv"),EFileWrite|EFileShareExclusive));CleanupClosePushL(result);User::LeaveIfError(result.Write(csv));User::LeaveIfError(result.Flush());CleanupStack::PopAndDestroy(&result);CleanupStack::PopAndDestroy(&csv);
    Log(_L("BENCH timings saved; API completion is not display confirmation"));
}
#endif
