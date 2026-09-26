// Research helper: copies a fixed list of ROM components to our data directory.
// Never opens the originals for writing, never changes repositories or processes.
#include <e32base.h>
#include <f32file.h>
#include <w32std.h>
#include <w32debug.h>
static RFs fs;
static void Log(const TDesC& text){
    RFile f;if(f.Open(fs,_L("C:\\data\\BelleWall\\probe.log"),EFileWrite|EFileShareAny)!=KErrNone)
        if(f.Create(fs,_L("C:\\data\\BelleWall\\probe.log"),EFileWrite|EFileShareAny)!=KErrNone)return;
    TInt pos=0;f.Seek(ESeekEnd,pos);TBuf8<512> line;line.Copy(text.Left(500));line.Append(_L8("\r\n"));f.Write(line);f.Flush();f.Close();
}
static void CopyL(const TDesC& name){
    TFileName original(_L("Z:\\sys\\bin\\"));original.Append(name);
    TFileName target(_L("C:\\data\\BelleWall\\snapshot\\"));target.Append(name);
    TEntry existing;if(fs.Entry(target,existing)==KErrNone){Log(_L("SKIP existing immutable snapshot"));return;}
    TFileName temporary(target);temporary.Append(_L(".partial"));
    RFile in;User::LeaveIfError(in.Open(fs,original,EFileRead|EFileShareReadersOnly));CleanupClosePushL(in);
    TInt length=0;User::LeaveIfError(in.Size(length));if(length<0||length>16*1024*1024)User::Leave(KErrTooBig);
    RFile out;User::LeaveIfError(out.Replace(fs,temporary,EFileWrite|EFileShareExclusive));CleanupClosePushL(out);
    HBufC8* buffer=HBufC8::NewLC(32768);TPtr8 bytes=buffer->Des();TInt copied=0;
    while(copied<length){User::LeaveIfError(in.Read(bytes,Min(32768,length-copied)));if(!bytes.Length())User::Leave(KErrCorrupt);User::LeaveIfError(out.Write(bytes));copied+=bytes.Length();}
    User::LeaveIfError(out.Flush());CleanupStack::PopAndDestroy(buffer);CleanupStack::PopAndDestroy(&out);CleanupStack::PopAndDestroy(&in);
    User::LeaveIfError(fs.Rename(temporary,target));
    TBuf<256> line;line.Format(_L("COPIED %S bytes=%d"),&name,copied);Log(line);
}
static void InspectWindowsL(){
    RWsSession ws;User::LeaveIfError(ws.Connect());CleanupClosePushL(ws);
    TBuf8<8192> windows;windows.SetMax();TInt result=ws.DebugInfo(EWsDebugSurfaceWindowList,windows,0);
    if(result>=0&&result<=windows.Length())windows.SetLength(result);
    TBuf<256> line;line.Format(_L("WINDOWS debug result=%d length=%d focus=%d"),result,windows.Length(),ws.GetFocusWindowGroup());Log(line);
    if(result>=0&&windows.Length()%sizeof(TWsDebugWindowId)==0){
        const TWsDebugWindowId* items=reinterpret_cast<const TWsDebugWindowId*>(windows.Ptr());
        for(TInt i=0;i<windows.Length()/sizeof(TWsDebugWindowId);++i){
            line.Format(_L("WINDOW index=%d client=%08x group=%d"),i,items[i].iClientId,items[i].iOtherGroupId);Log(line);
        }
    }
    CleanupStack::PopAndDestroy(&ws);
    const TPtrC patterns[]={_L("C:\\sys\\bin\\*.ldd"),_L("Z:\\sys\\bin\\*patch*"),_L("C:\\sys\\bin\\*patch*"),_L("Z:\\sys\\bin\\alf*"),_L("Z:\\sys\\bin\\*hitch*")};
    for(TUint i=0;i<sizeof(patterns)/sizeof(patterns[0]);++i){
        CDir* entries=0;TInt error=fs.GetDir(patterns[i],KEntryAttNormal,ESortByName,entries);
        line.Format(_L("INVENTORY %S error=%d"),&patterns[i],error);Log(line);
        if(!error){for(TInt j=0;j<entries->Count();++j)Log((*entries)[j].iName);delete entries;}
    }
}
static void InventoryTreeL(const TDesC& path,TInt depth,TInt& count){
    if(depth<0||count>=2500)return;CDir* entries=0;TInt error=fs.GetDir(path,KEntryAttDir,ESortByName,entries);TBuf<400> line;line.Format(_L("HS DIR %S error=%d"),&path,error);Log(line);if(error)return;CleanupStack::PushL(entries);
    for(TInt i=0;i<entries->Count()&&count<2500;i++){const TEntry& item=(*entries)[i];TFileName full(path);full.Append(item.iName);line.Format(_L("HS ENTRY %S dir=%d bytes=%d"),&full,item.IsDir(),item.iSize);Log(line);count++;if(item.IsDir()){full.Append(_L("\\"));InventoryTreeL(full,depth-1,count);}}
    CleanupStack::PopAndDestroy(entries);
}
static void RunL(){
    User::LeaveIfError(fs.Connect());CleanupClosePushL(fs);
    TInt err=fs.MkDirAll(_L("C:\\data\\BelleWall\\snapshot\\"));if(err!=KErrNone&&err!=KErrAlreadyExists)User::Leave(err);
    TBuf<64> args;User::CommandLine(args);if(args.Find(_L("--hs-manifest"))!=KErrNotFound){RFile file;User::LeaveIfError(file.Open(fs,_L("Z:\\private\\200159c0\\install\\posterwideimage_2001fdbc\\hsps\\00\\manifest.dat"),EFileRead|EFileShareReadersOnly));CleanupClosePushL(file);TBuf8<2048> bytes;User::LeaveIfError(file.Read(bytes));for(TInt i=0;i<bytes.Length();i+=200){TBuf<200> line;line.Copy(bytes.Mid(i,Min(200,bytes.Length()-i)));Log(line);}CleanupStack::PopAndDestroy(&file);CleanupStack::PopAndDestroy(&fs);return;}if(args.Find(_L("--hs-inventory"))!=KErrNotFound){TInt count=0;InventoryTreeL(_L("Z:\\private\\200159c0\\"),4,count);InventoryTreeL(_L("C:\\private\\200159c0\\"),3,count);Log(_L("HS inventory done; metadata only"));CleanupStack::PopAndDestroy(&fs);return;}
    Log(_L("BEGIN ROM snapshot; fixed file list; read-only sources; AllFiles research helper"));
    const TPtrC names[]={_L("alfdecoderserverclient.dll"),_L("alfappservercore.dll"),_L("alfrenderstage.dll"),_L("alfcompositorrs.dll"),_L("aknskins.dll"),_L("aknskinsrv.dll"),_L("xn3layoutengine.dll"),_L("extrenderingplugin.dll"),_L("hspsclient.dll"),_L("hspsdefrep.dll"),_L("hspsthemeserver.exe"),_L("hspsclientsession.dll"),_L("hsccapiclient.dll")};
    for(TUint i=0;i<sizeof(names)/sizeof(names[0]);++i){TRAPD(error,CopyL(names[i]));if(error){TBuf<256> line;line.Format(_L("ERROR %S code=%d"),&names[i],error);Log(line);}}
    Log(_L("END ROM snapshot"));InspectWindowsL();CleanupStack::PopAndDestroy(&fs);
}
GLDEF_C TInt E32Main(){CTrapCleanup* cleanup=CTrapCleanup::New();if(!cleanup)return KErrNoMemory;TRAPD(err,RunL());delete cleanup;return err;}
