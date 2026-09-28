#ifndef BELLEWALL_SHAREDWALLPAPER_H
#define BELLEWALL_SHAREDWALLPAPER_H
#include <AknsSrvClient.h>
#include <bitdev.h>
#include <bitstd.h>
#include "redrawtransport.h"
class LiveCacheSession:public RAknsSrvSession{
public:TInt BitmapHandleL(const TDesC& path,const TSize& target=TSize(360,640)){TPckgBuf<TInt> bitmap,mask;bitmap()=0;mask()=0;TPckgC<TSize> size(target);TInt result=SendReceive(21,TIpcArgs(&path,&size,&bitmap,&mask));TBuf<160> detail;detail.Format(_L("CACHE lookup result=%d bitmap=%d mask=%d"),result,bitmap(),mask());Log(detail);User::LeaveIfError(result);if(!bitmap()||mask())User::Leave(KErrNotSupported);return bitmap();}
};
class LiveCacheImage:public CBase{
public:
    static LiveCacheImage* NewLC(const TDesC& path,const TSize& size=TSize(360,640)){LiveCacheImage* self=new(ELeave)LiveCacheImage;CleanupStack::PushL(self);self->target=size;User::LeaveIfError(self->session.Connect());self->bitmap=new(ELeave)CFbsBitmap;User::LeaveIfError(self->bitmap->Duplicate(self->session.BitmapHandleL(path,self->target)));if(self->bitmap->SizeInPixels()!=self->target||self->bitmap->IsCompressedInRAM())User::Leave(KErrNotSupported);self->device=CFbsBitmapDevice::NewL(self->bitmap);User::LeaveIfError(self->device->CreateContext(self->gc));TBuf<160> line;line.Format(_L("SHARED cache handle=%d size=%dx%d mode=%d"),self->bitmap->Handle(),size.iWidth,size.iHeight,self->bitmap->DisplayMode());Log(line);return self;}
    ~LiveCacheImage(){if(publisher){TBuf<140> line;line.Format(_L("DIRTY published=%d unchanged=%d partial=%d"),published,unchanged,partial);Log(line);}delete previous;delete publisher;delete gc;delete device;delete bitmap;session.Close();}
    void EnableDirtyL(){publisher=CRedrawPublisher::NewL();publisher->TargetL(target);previous=HBufC8::NewL(180*320*2);Log(_L("DIRTY rectangle publication enabled; merge unacknowledged regions"));}
    TBool CompressedTarget()const{return bitmap&&bitmap->SizeInPixels()==target&&bitmap->IsCompressedInRAM();}
    // Refresh the FBS reference and reject a compressed target before drawing.
    // This is a defensive check, not a lock against other clients' compression.
    TBool BeginPaintL(){if(publisher&&!publisher->BeginWrite())return EFalse;bitmap->BeginDataAccess();if(bitmap->IsCompressedInRAM()){bitmap->EndDataAccess(ETrue);if(publisher)publisher->EndWrite();Log(_L("FBS compressed target rejected before drawing"));User::Leave(KErrNotSupported);}return ETrue;}
    void EndPaint(){bitmap->EndDataAccess(EFalse);if(publisher)publisher->EndWrite();}
    TSize Size()const{return target;}
    TBool NeedsRebuildL(){return publisher&&publisher->NeedsRebuildL();}
    TSize ObservedL(){return publisher?publisher->ObservedL():target;}
    void Configure(TInt mode,TRgb color){fit=mode;background=color;previousValid=EFalse;}
    TUint GeometryL(){return publisher?publisher->GeometryL():0;}
    void ResizeL(const TDesC& path,const TSize& size,TUint epoch){target=size;RefreshL(path);if(publisher)publisher->TargetL(target,epoch);}
    void RefreshL(const TDesC& path){
        Log(_L("RESUME reacquire FBS target begin"));
        delete gc;gc=0;delete device;device=0;delete bitmap;bitmap=0;session.Close();
        User::LeaveIfError(session.Connect());bitmap=new(ELeave)CFbsBitmap;User::LeaveIfError(bitmap->Duplicate(session.BitmapHandleL(path,target)));
        TBuf<160> detail;detail.Format(_L("RESUME target handle=%d size=%dx%d mode=%d compressed=%d"),bitmap->Handle(),bitmap->SizeInPixels().iWidth,bitmap->SizeInPixels().iHeight,bitmap->DisplayMode(),bitmap->IsCompressedInRAM());Log(detail);
        if(bitmap->SizeInPixels()!=target||bitmap->IsCompressedInRAM())User::Leave(KErrNotSupported);
        Log(_L("RESUME creating bitmap device"));device=CFbsBitmapDevice::NewL(bitmap);User::LeaveIfError(device->CreateContext(gc));previousValid=EFalse;markerValid=EFalse;
        Log(_L("RESUME reacquire FBS target complete"));
    }
    void Blit(CFbsBitmap& source){
        TRect dirty(TPoint(0,0),target);
        if(target==TSize(360,640)&&fit==BelleDisplay::Cover&&publisher&&source.SizeInPixels()==TSize(180,320)&&source.DisplayMode()==EColor64K){
            source.BeginDataAccess();const TUint8* bytes=reinterpret_cast<const TUint8*>(source.DataAddress());const TInt stride=CFbsBitmap::ScanLineLength(180,EColor64K);TPtr8 old=previous->Des();
            if(previousValid){TInt minX=180,minY=320,maxX=-1,maxY=-1;for(TInt y=0;y<320;y++){const TUint16* row=reinterpret_cast<const TUint16*>(bytes+y*stride);const TUint16* last=reinterpret_cast<const TUint16*>(old.Ptr()+y*360);for(TInt x=0;x<180;x++)if(row[x]!=last[x]){minX=Min(minX,x);maxX=Max(maxX,x);minY=Min(minY,y);maxY=Max(maxY,y);}}if(maxX<0)dirty=TRect();else {dirty=TRect(minX*2,minY*2,(maxX+1)*2,(maxY+1)*2);dirty.Grow(2,2);dirty.Intersection(TRect(0,0,360,640));}}
            old.SetLength(180*320*2);for(TInt y=0;y<320;y++)Mem::Copy(const_cast<TUint8*>(old.Ptr())+y*360,bytes+y*stride,360);previousValid=ETrue;source.EndDataAccess(ETrue);
        }
        if(dirty.IsEmpty()){unchanged++;return;}if(!BeginPaintL()){previousValid=EFalse;return;}const BelleDisplay::Mapping map=BelleDisplay::Map(source.SizeInPixels().iWidth,source.SizeInPixels().iHeight,target.iWidth,target.iHeight,fit);gc->SetBrushStyle(CGraphicsContext::ESolidBrush);gc->SetBrushColor(background);gc->SetPenStyle(CGraphicsContext::ENullPen);if(fit==BelleDisplay::Contain)gc->DrawRect(TRect(TPoint(0,0),target));gc->DrawBitmap(TRect(map.dx,map.dy,map.dx+map.dw,map.dy+map.dh),&source,TRect(map.sx,map.sy,map.sx+map.sw,map.sy+map.sh));EndPaint();Publish(dirty);
    }
    void Paint(TInt frame){if(!BeginPaintL())return;TInt before=bitmap->TouchCount();gc->SetPenStyle(CGraphicsContext::ENullPen);gc->SetBrushStyle(CGraphicsContext::ESolidBrush);gc->SetBrushColor(TRgb(8,20,35));gc->DrawRect(TRect(TPoint(0,0),target));gc->SetBrushColor(TRgb(25,210,150));TInt x=(frame*13)%296;gc->DrawRect(TRect(x,280,x+64,344));TRect dirty(TPoint(0,0),target);if(markerValid)dirty=TRect(Min(lastX,x),280,Max(lastX,x)+64,344);lastX=x;markerValid=ETrue;EndPaint();Publish(dirty);if(!frame){TBuf<100> line;line.Format(_L("SHARED touch before=%d after=%d volatile=%d"),before,bitmap->TouchCount(),bitmap->IsVolatile());Log(line);}}
    void Publish(const TRect& dirty){if(publisher){publisher->Publish(dirty);published++;if(dirty.Width()*dirty.Height()<target.iWidth*target.iHeight)partial++;}}
private:LiveCacheImage():target(360,640),fit(BelleDisplay::Cover),background(0,0,0),bitmap(0),device(0),gc(0),publisher(0),previous(0),previousValid(EFalse),markerValid(EFalse),lastX(0),published(0),unchanged(0),partial(0){}TSize target;TInt fit;TRgb background;LiveCacheSession session;CFbsBitmap* bitmap;CFbsBitmapDevice* device;CFbsBitGc* gc;CRedrawPublisher* publisher;HBufC8* previous;TBool previousValid;TBool markerValid;TInt lastX;TInt published;TInt unchanged;TInt partial;
};
static void SharedSpeedL(RWsSession& ws,CHWRMLight& light,LiveCacheImage& image,const TDesC& path,TBool flushTest,TBool localRedraw=EFalse){
    BenchClock clock;clock.InitL();const TUint began=clock.Now();
    if(localRedraw)Log(_L("LOCAL BACKGROUND writer: no per-frame global invalidation or Finish; desktop plugin drives redraw"));
    const TInt group=ws.GetFocusWindowGroup();CRepository* repo=CRepository::NewLC(KCRUidPersonalisation);
    BenchmarkScreen* observer=BenchmarkScreen::NewLC(ws);
    RBuf8 csv;csv.CreateL(128*1024);CleanupClosePushL(csv);
    csv.Append(_L8("phase,target_us,index,gate_us,paint_us,redraw_us,sample_us,cycle_us,elapsed_us,marker_x,marker_count,clear_us,sync_us\n"));
    TInt targets[]={200000,100000,50000,33333,100000};if(flushTest){targets[0]=33333;targets[1]=33333;targets[2]=16666;targets[3]=33333;}TInt serial=0;TBool stop=EFalse;
    for(TInt phase=0;phase<5&&!stop;phase++){
        TUint start=clock.Now();TInt count=0;
        while(clock.Us(start)<3000000&&count<160){
            TUint cycle=clock.Now();TEntry entry;TFileName current;
            User::LeaveIfError(repo->Get(KPslnIdleBackgroundImagePath,current));
            if(clock.Us(began)>22000000||fs.Entry(KStop,entry)==KErrNone||current.CompareF(path)||!BenchGate(ws,light,group)){stop=ETrue;break;}
            TInt gateUs=clock.Us(cycle);TUint paint=clock.Now();image.Paint(serial++);TInt paintUs=clock.Us(paint);
            TUint redraw=clock.Now();if(!localRedraw)ws.ClearAllRedrawStores();TInt clearUs=clock.Us(redraw);TUint sync=clock.Now();
            if(!localRedraw){if(flushTest&&phase>0)ws.Flush();else User::LeaveIfError(ws.Finish());}TInt syncUs=clock.Us(sync),redrawUs=clock.Us(redraw);
            TInt sampleUs=0,x=-1,pixels=0;if(phase==4||(flushTest&&phase==3&&count%6==0)){TUint sample=clock.Now();x=observer->SampleL(pixels);sampleUs=clock.Us(sample);}
            // Absolute phase deadlines avoid accumulating timer rounding each frame.
            // If work exceeds the budget, do not wait; the bounded phase still ends.
            TInt remaining=(count+1)*targets[phase]-clock.Us(start);if(remaining>0)User::After(remaining);
            TBuf8<220> row;row.Format(_L8("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n"),phase,targets[phase],count,gateUs,paintUs,redrawUs,sampleUs,clock.Us(cycle),clock.Us(start),x,pixels,clearUs,syncUs);
            if(csv.MaxLength()-csv.Length()<row.Length()){stop=ETrue;break;}csv.Append(row);++count;
        }
        TBuf<120> line;line.Format(_L("SHARED SPEED phase=%d target_us=%d frames=%d elapsed_us=%d"),phase,targets[phase],count,clock.Us(start));Log(line);
        TUint drain=clock.Now();User::LeaveIfError(ws.Finish());line.Format(_L("SHARED SPEED phase=%d drain_us=%d flush_test=%d"),phase,clock.Us(drain),flushTest);Log(line);
    }
    RFile result;User::LeaveIfError(result.Replace(fs,_L("C:\\data\\BelleWall\\wallpaper-benchmark.csv"),EFileWrite|EFileShareExclusive));CleanupClosePushL(result);User::LeaveIfError(result.Write(csv));User::LeaveIfError(result.Flush());CleanupStack::PopAndDestroy(&result);
    CleanupStack::PopAndDestroy(&csv);CleanupStack::PopAndDestroy(observer);CleanupStack::PopAndDestroy(repo);
    Log(_L("SHARED SPEED saved; submission rate is not LCD presentation rate"));
}
class ContentFrames:public CBase{
public:
    static ContentFrames* NewLC(){ContentFrames* self=new(ELeave)ContentFrames;CleanupStack::PushL(self);
        TFileName path(_L("C:\\data\\BelleWall\\video-frames.bin"));TBool compressed=EFalse;RFile selected;TInt selectedError=selected.Open(fs,_L("C:\\data\\BelleWall\\selected-wallpaper.txt"),EFileRead|EFileShareReadersOnly);if(selectedError==KErrNotFound)selectedError=selected.Open(fs,_L("C:\\data\\BelleWall\\selected-video.txt"),EFileRead|EFileShareReadersOnly);if(!selectedError){TBuf8<80> name;TInt read=selected.Read(name);selected.Close();User::LeaveIfError(read);TInt prefix=name.Length()==70?2:0;if(name.Length()!=68+prefix||(name.Right(4)!=_L8(".bwv")&&name.Right(4)!=_L8(".mp4")))User::Leave(KErrCorrupt);compressed=name.Right(4)==_L8(".mp4");TUint8 drive='C';if(prefix){drive=name[0];if((drive!='C'&&drive!='E'&&drive!='F')||name[1]!=':')User::Leave(KErrCorrupt);}for(TInt i=prefix;i<prefix+64;i++)if(!((name[i]>='0'&&name[i]<='9')||(name[i]>='a'&&name[i]<='f')))User::Leave(KErrCorrupt);path=_L("C:\\data\\BelleWall\\library\\");path[0]=drive;TBuf<80> wide;wide.Copy(name.Mid(prefix));path.Append(wide);}else if(selectedError!=KErrNotFound)User::Leave(selectedError);
        if(compressed){self->native=NativeVideoFrames::NewL();self->native->OpenL(path);const TSize size=self->native->FrameSize();const TReal32 rate=self->native->FrameRate();const TInt fps=TInt(rate+0.5f);if(!BelleDisplay::Valid(size.iWidth,size.iHeight)||(fps!=10&&fps!=20&&fps!=30)||rate<fps-0.2f||rate>fps+0.2f||self->native->DurationUs()>TInt64(100000)*1000000/fps)User::Leave(KErrNotSupported);self->width=size.iWidth;self->height=size.iHeight;self->fps=fps;self->den=1;self->count=TInt((self->native->DurationUs()*fps+999999)/1000000);if(self->count<1||self->count>100000)User::Leave(KErrCorrupt);TBuf<128> line;line.Format(_L("CONTENT MP4 candidate decoder=%dx%d fps=%d frames=%d"),self->width,self->height,self->fps,self->count);Log(line);return self;}
        User::LeaveIfError(self->file.Open(fs,path,EFileRead|EFileShareReadersOnly));
        TUint fields[6];TPtr8 header(reinterpret_cast<TUint8*>(fields),24,24);User::LeaveIfError(self->file.Read(header));
        if(header.Length()!=24)User::Leave(KErrCorrupt);
        if(fields[0]==0x31565742){if(fields[1]!=180||fields[2]!=320||fields[3]!=30||fields[4]!=15||fields[5]!=115200)User::Leave(KErrCorrupt);self->count=30;self->fps=15;self->den=1;self->offset=24;self->width=180;self->height=320;self->frameBytes=115200;}
        else if(fields[0]==0x32565742){TUint tail[6];TPtr8 extra(reinterpret_cast<TUint8*>(tail),24,24);User::LeaveIfError(self->file.Read(extra));if(extra.Length()!=24||fields[1]!=48||(!BelleDisplay::Valid(fields[2],fields[3])||(fields[2]&1))||fields[4]!=fields[2]*2||fields[5]!=1||!tail[0]||tail[0]>60000||!tail[1]||tail[1]>1001||tail[0]<tail[1]||tail[0]>60*tail[1]||!tail[2]||tail[2]>268435456/(fields[4]*fields[3])||tail[3]!=fields[4]*fields[3]||tail[4]!=48||tail[5]!=tail[2]*tail[3])User::Leave(KErrCorrupt);self->count=tail[2];self->fps=tail[0];self->den=tail[1];self->offset=48;self->width=fields[2];self->height=fields[3];self->frameBytes=tail[3];}
        else User::Leave(KErrNotSupported);
        TInt size;User::LeaveIfError(self->file.Size(size));if(size!=self->offset+self->frameBytes*self->count)User::Leave(KErrCorrupt);
        self->bitmap=new(ELeave)CFbsBitmap;User::LeaveIfError(self->bitmap->Create(TSize(self->width,self->height),EColor64K));return self;
    }
    ~ContentFrames(){delete native;delete bitmap;file.Close();}
    CFbsBitmap& ReadL(TInt index){if(index<0)User::Leave(KErrArgument);if(native){CFbsBitmap& frame=native->FrameAtUsL(TInt64(index%count)*1000000*den/fps);if(frame.SizeInPixels()!=TSize(width,height))User::Leave(KErrNotSupported);return frame;}bitmap->BeginDataAccess();TPtr8 bytes(reinterpret_cast<TUint8*>(bitmap->DataAddress()),frameBytes,frameBytes);TInt error=file.Read(offset+(index%count)*frameBytes,bytes);bitmap->EndDataAccess(EFalse);User::LeaveIfError(error);if(bytes.Length()!=frameBytes)User::Leave(KErrCorrupt);return *bitmap;}
    TInt count,fps,den,offset,width,height,frameBytes;
private:ContentFrames():count(0),fps(0),den(1),offset(0),width(0),height(0),frameBytes(0),bitmap(0),native(0){}RFile file;CFbsBitmap* bitmap;NativeVideoFrames* native;
};
static void SharedContentL(RWsSession& ws,CHWRMLight& light,LiveCacheImage& image,const TDesC& path,TBool localRedraw=EFalse){
    ContentFrames* frames=ContentFrames::NewLC();CRepository* repo=CRepository::NewLC(KCRUidPersonalisation);
    RBuf8 csv;csv.CreateL(64*1024);CleanupClosePushL(csv);csv.Append(_L8("phase,target_us,index,source_frame,read_us,blit_us,redraw_us,cycle_us,elapsed_us\n"));
    BenchClock clock;clock.InitL();const TInt group=ws.GetFocusWindowGroup();TBool stop=EFalse;TInt targets[]={100000,66667};
    if(localRedraw)Log(_L("LOCAL BACKGROUND real video: no per-frame global redraw"));
    Log(_L("CONTENT real video RGB565 180x320 to 360x640; 2-second loop; no video decoder"));
    for(TInt phase=0;phase<2&&!stop;phase++){
        TUint start=clock.Now();TInt count=0;
        while(clock.Us(start)<5000000&&count<100){
            TUint cycle=clock.Now();TEntry entry;TFileName current;User::LeaveIfError(repo->Get(KPslnIdleBackgroundImagePath,current));
            if(fs.Entry(KStop,entry)==KErrNone||current.CompareF(path)||!BenchGate(ws,light,group)){stop=ETrue;break;}
            TInt source=(TInt64(clock.Us(start))*15/1000000)%30;TUint read=clock.Now();CFbsBitmap& frame=frames->ReadL(source);TInt readUs=clock.Us(read);
            TUint blit=clock.Now();image.Blit(frame);TInt blitUs=clock.Us(blit);TUint redraw=clock.Now();if(!localRedraw){ws.ClearAllRedrawStores();User::LeaveIfError(ws.Finish());}TInt redrawUs=clock.Us(redraw);
            TInt remaining=(count+1)*targets[phase]-clock.Us(start);if(remaining>0)User::After(remaining);
            TBuf8<180> row;row.Format(_L8("%d,%d,%d,%d,%d,%d,%d,%d,%d\n"),phase,targets[phase],count,source,readUs,blitUs,redrawUs,clock.Us(cycle),clock.Us(start));csv.Append(row);count++;
        }
        TBuf<120> line;line.Format(_L("CONTENT phase=%d target_us=%d frames=%d elapsed_us=%d"),phase,targets[phase],count,clock.Us(start));Log(line);
    }
    RFile result;User::LeaveIfError(result.Replace(fs,_L("C:\\data\\BelleWall\\wallpaper-benchmark.csv"),EFileWrite|EFileShareExclusive));CleanupClosePushL(result);User::LeaveIfError(result.Write(csv));User::LeaveIfError(result.Flush());CleanupStack::PopAndDestroy(&result);
    CleanupStack::PopAndDestroy(&csv);CleanupStack::PopAndDestroy(repo);CleanupStack::PopAndDestroy(frames);
}
static TUint BmpU32(const TUint8* p){return TUint(p[0])|(TUint(p[1])<<8)|(TUint(p[2])<<16)|(TUint(p[3])<<24);}
static void ReadWebBitmapL(const TDesC& path,CFbsBitmap& bitmap){
    RFile file;User::LeaveIfError(file.Open(fs,path,EFileRead|EFileShareReadersOnly));CleanupClosePushL(file);TInt size;User::LeaveIfError(file.Size(size));if(size<54||size>240000)User::Leave(KErrCorrupt);
    RBuf8 data;data.CreateL(size);CleanupClosePushL(data);User::LeaveIfError(file.Read(data));if(data.Length()!=size)User::Leave(KErrCorrupt);
    const TUint8* p=data.Ptr();TUint offset=BmpU32(p+10),bits=p[28]|(p[29]<<8);TInt stride=((180*bits+31)/32)*4;
    if(p[0]!='B'||p[1]!='M'||BmpU32(p+14)<40||BmpU32(p+18)!=180||BmpU32(p+22)!=320||p[26]!=1||p[27]!=0||(bits!=24&&bits!=32)||BmpU32(p+30)!=0||offset<54||offset>TUint(size)||TUint(stride*320)>TUint(size)-offset)User::Leave(KErrNotSupported);
    bitmap.BeginDataAccess();TUint8* dest=reinterpret_cast<TUint8*>(bitmap.DataAddress());const TInt destStride=CFbsBitmap::ScanLineLength(180,EColor16M);
    for(TInt y=0;y<320;y++){const TUint8* src=p+offset+(319-y)*stride;TUint8* row=dest+y*destStride;for(TInt x=0;x<180;x++){row[x*3]=src[x*(bits/8)];row[x*3+1]=src[x*(bits/8)+1];row[x*3+2]=src[x*(bits/8)+2];}}
    bitmap.EndDataAccess(EFalse);CleanupStack::PopAndDestroy(&data);CleanupStack::PopAndDestroy(&file);
}
static void SharedWebL(RWsSession& ws,CHWRMLight& light,LiveCacheImage& image,const TDesC& path){
    WebProducer* producer=new(ELeave)WebProducer;CleanupStack::PushL(producer);producer->StartL(EFalse);
    CFbsBitmap* bitmap=new(ELeave)CFbsBitmap;CleanupStack::PushL(bitmap);User::LeaveIfError(bitmap->Create(TSize(180,320),EColor16M));
    TFileName warm;producer->FrameL(0,warm);ReadWebBitmapL(warm,*bitmap);Log(_L("SHARED WEB warmed; waiting for native desktop"));
    TEntry entry;for(TInt wait=0;!DesktopReadyL(ws,light);wait++){if(wait>=50||fs.Entry(KStop,entry)==KErrNone)User::Leave(KErrCancel);User::After(100000);}
    BenchClock clock;clock.InitL();TUint start=clock.Now();TInt count=0;
    Log(_L("SHARED WEB live phone WebKit via existing BMP exporter; not a peak FPS benchmark"));
    while(clock.Us(start)<20000000&&count<100){
        TFileName current;CurrentL(current);if(fs.Entry(KStop,entry)==KErrNone||current.CompareF(path)||!DesktopReadyL(ws,light))break;
        Status(_L("SHARED WEB requesting frame"),count);TFileName frame;TRAPD(exportError,producer->FrameL((count+1)%20,frame));if(exportError){Status(_L("SHARED WEB original export error"),exportError);User::Leave(exportError);}Log(_L("SHARED WEB reading BMP"));ReadWebBitmapL(frame,*bitmap);Log(_L("SHARED WEB BMP ready"));
        CurrentL(current);if(current.CompareF(path)||!DesktopReadyL(ws,light))break;
        image.Blit(*bitmap);ws.ClearAllRedrawStores();User::LeaveIfError(ws.Finish());count++;Status(_L("SHARED WEB displayed request completed"),count);
    }
    TBuf<120> line;line.Format(_L("SHARED WEB frames=%d elapsed_us=%d"),count,clock.Us(start));Log(line);
    CleanupStack::PopAndDestroy(bitmap);CleanupStack::PopAndDestroy(producer);
}
#include "sharedwebstream.h"
static void SharedWallpaperL(RWsSession& ws,CHWRMLight& light,const TDesC& original,TInt mode){
    const TBool redraw=mode==7,speed=mode>=8;
    TEntry entry;TInt waiting=0;while(!DesktopReadyL(ws,light)){if(++waiting>100||fs.Entry(KStop,entry)==KErrNone)User::Leave(KErrCancel);User::After(100000);}
    TFileName current;CurrentL(current);if(current.CompareF(original))User::Leave(KErrInUse);
    TFileName path;FrameL(mode==17?820:mode==16?810:mode==15?800:mode==14?790:mode==13?780:mode==12?770:mode==11?760:mode==10?750:mode==9?740:speed?731:redraw?720:700,path,0,360,640);if(mode==13||mode==17)BindPagesL(path);User::LeaveIfError(AknsWallpaperUtils::SetIdleWallpaper(path,0));Log(_L("SHARED wallpaper selected ONCE; subsequent frames modify cache only"));User::After(1500000);
    User::LeaveIfError(RFbsSession::Connect());LiveCacheImage* image=LiveCacheImage::NewLC(path);if(mode>=14)image->EnableDirtyL();
    if(mode==16||mode==17)SharedWebStreamL(ws,light,*image,path,ETrue);else if(mode==15)SharedContentL(ws,light,*image,path,ETrue);else if(mode==13)SharedWebStreamL(ws,light,*image,path);else if(mode==12)SharedWebStreamL(ws,light,*image,path);else if(mode==11)SharedWebL(ws,light,*image,path);else if(mode==10)SharedContentL(ws,light,*image,path);else if(speed)SharedSpeedL(ws,light,*image,path,mode==9,mode==14);
    TInt painted=0;for(TInt tick=0;!speed&&tick<(redraw?12:120);tick++){
        if(fs.Entry(KStop,entry)==KErrNone)break;
        if(DesktopReadyL(ws,light)){
            CurrentL(current);if(current.CompareF(path))break;
            image->Paint(redraw?(painted%2)*20:painted);painted++;
            if(redraw){
                ws.ClearAllRedrawStores();User::LeaveIfError(ws.Finish());
                TBuf<100> line;line.Format(_L("SHARED STEP %d x=%d redraw completed (maximum 12)"),painted,((painted-1)%2)*260);Log(line);
            }else if(painted%20==0){TBuf<80> line;line.Format(_L("SHARED painted=%d; display not yet confirmed"),painted);Log(line);}
        }
        User::After(redraw?1000000:100000);
    }
    CleanupStack::PopAndDestroy(image);RFbsSession::Disconnect();Log(_L("SHARED writer done; restore follows"));
}
#endif
