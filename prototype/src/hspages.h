#ifndef BELLEWALL_HSPAGES_H
#define BELLEWALL_HSPAGES_H
#include <hspswrapper.h>
#include <hspsconfiguration.h>
#include <pluginmap.h>
#include <itemmap.h>
#include <propertymap.h>
#include <s32file.h>
#include <hsccapiclient.h>
#include <hscontentcontrol.h>
#include <hscontentinfo.h>
#include <hscontentinfoarray.h>
#include <activeidle2domaincrkeys.h>
#include <AknsSrvClient.h>
using namespace hspswrapper;
_LIT(KPagesJournal,"C:\\data\\BelleWall\\pages-rollback.bin");
const TUint KPagesJournalMagic=0x42575033;
struct PageBackup { TBuf8<32> id; TBuf8<512> original; };
static CPropertyMap* PagePathL(CHspsConfiguration& config){
    RPointerArray<CItemMap>& items=config.Settings();
    for(TInt i=0;i<items.Count();i++)if(items[i]->ItemId()==_L8("wallpaper")){
        RPointerArray<CPropertyMap>& props=items[i]->Properties();
        for(TInt j=0;j<props.Count();j++)if(props[j]->Name()==_L8("path"))return props[j];
    }
    User::Leave(KErrNotFound);return 0;
}
static CHspsWrapper* PagesClientLC(){TBuf8<20> uid;uid.Num(0x102750f0);return CHspsWrapper::NewLC(uid);}
static TBool ProjectPage(const TDesC8& path){TEntry e;if(fs.Entry(KCandidateJournal,e)==KErrNone){TCandidateRecord r;CandidateReadL(fs,r);TBuf<16> token;CandidateToken(r,token);TFileName expected;expected.Format(_L("C:\\data\\BelleWall\\native-frame-%S-830.bmp"),&token);TBuf8<256> bytes;bytes.Copy(expected);return path.CompareF(bytes)==0;}return path.Left(_L8("C:\\data\\BelleWall\\native-frame-").Length()).CompareF(_L8("C:\\data\\BelleWall\\native-frame-"))==0;}
static void PageSetL(CHspsWrapper& hs,const TDesC8& id,const TDesC8& value){
    CHspsConfiguration* config=hs.GetPluginConfigurationL(id);if(!config)User::Leave(KErrNotFound);CleanupStack::PushL(config);
    PagePathL(*config)->SetValueL(value);User::LeaveIfError(hs.SetPluginSettingsL(id,config->Settings()));CleanupStack::PopAndDestroy(config);
}
static void PageActiveL(TDes8& id){CRepository* repo=CRepository::NewLC(TUid::Uid(KCRUidActiveIdleLV));User::LeaveIfError(repo->Get(KAIActiveViewPluginId,id));CleanupStack::PopAndDestroy(repo);}
class TPagesObserver:public MHsContentControl{public:void NotifyWidgetListChanged(){}void NotifyViewListChanged(){}void NotifyAppListChanged(){}};
static void PageActivateL(CHspsWrapper&,const TDesC8& id){
    TBuf8<32> current;PageActiveL(current);if(current==id)return;
    Log(_L("PAGE API creating content client"));TPagesObserver observer;CHsCcApiClient* client=CHsCcApiClient::NewL(&observer);CleanupStack::PushL(client);MHsContentController* control=client;Log(_L("PAGE API created"));
    CHsContentInfoArray* views=CHsContentInfoArray::NewL();CleanupStack::PushL(views);CHsContentInfo* app=CHsContentInfo::NewLC();app->SetTypeL(_L8("application"));app->SetUidL(_L8("0x2001f482"));TInt listError=control->ViewListL(*app,*views);CleanupStack::PopAndDestroy(app);Status(_L("PAGE API view list result"),listError);User::LeaveIfError(listError);Status(_L("PAGE API view count"),views->Array().Count());
    TBool found=EFalse;for(TInt i=0;i<views->Array().Count();i++)if(views->Array()[i]->PluginId()==id){User::LeaveIfError(control->ActivateViewL(*views->Array()[i]));found=ETrue;break;}
    if(!found)User::Leave(KErrNotFound);CleanupStack::PopAndDestroy(views);CleanupStack::PopAndDestroy(client);
    for(TInt i=0;i<40;i++){PageActiveL(current);if(current==id){User::After(250000);return;}User::After(50000);}
    TBuf<100> failure;TBuf<32> wanted,observed;wanted.Copy(id);observed.Copy(current);failure.Format(_L("PAGE activation timeout requested=%S observed=%S"),&wanted,&observed);Log(failure);User::Leave(KErrTimedOut);
}
static void RestorePagesL(){
    TEntry entry;TInt exists=fs.Entry(KPagesJournal,entry);if(exists==KErrNotFound)return;User::LeaveIfError(exists);
    CandidateVerifyL(fs,KPagesJournal);
    RFileReadStream in;User::LeaveIfError(in.Open(fs,KPagesJournal,EFileRead|EFileShareReadersOnly));CleanupClosePushL(in);
    TUint magic=in.ReadUint32L();if(magic!=KPagesJournalMagic){TEntry session;if(magic!=0x42575032||fs.Entry(KCandidateJournal,session)!=KErrNone)User::Leave(KErrCorrupt);Log(_L("PAGES accepting checksum-verified r2/r3 candidate journal"));}TInt count=in.ReadInt32L();if(count<1||count>8)User::Leave(KErrCorrupt);
    TBuf8<32> initial;in>>initial;PageBackup pages[8];for(TInt i=0;i<count;i++){in>>pages[i].id;in>>pages[i].original;}CleanupStack::PopAndDestroy(&in);
    CHspsWrapper* hs=PagesClientLC();TBool conflict=EFalse;
    for(TInt i=0;i<count;i++){
        CHspsConfiguration* config=hs->GetPluginConfigurationL(pages[i].id);if(!config)User::Leave(KErrNotFound);CleanupStack::PushL(config);
        TBuf8<512> current(PagePathL(*config)->Value());CleanupStack::PopAndDestroy(config);
        if(current!=pages[i].original&&!ProjectPage(current)){Log(_L("PAGES restore conflict; user path preserved; journal retained"));conflict=ETrue;continue;}
        PageSetL(*hs,pages[i].id,pages[i].original);
        PageActivateL(*hs,pages[i].id);TFileName original;original.Copy(pages[i].original);User::LeaveIfError(AknsWallpaperUtils::SetIdleWallpaper(original,0));User::After(300000);
    }
    PageActivateL(*hs,initial);User::After(500000);
    for(TInt i=0;i<count;i++){
        CHspsConfiguration* config=hs->GetPluginConfigurationL(pages[i].id);if(!config)User::Leave(KErrNotFound);CleanupStack::PushL(config);
        if(PagePathL(*config)->Value()!=pages[i].original)conflict=ETrue;CleanupStack::PopAndDestroy(config);
    }
    CleanupStack::PopAndDestroy(hs);if(conflict)User::Leave(KErrInUse);
    User::LeaveIfError(fs.Delete(KPagesJournal));Log(_L("PAGES all saved page paths restored and verified"));
}
// Resume only: keep the original backup intact and reject any user-changed page.
// Reapply the same session path after dropping only that path's decoded cache.
// Never activate a page here: unlock must preserve the user's current page.
static void RefreshCandidatePagesL(const TDesC& path){
    TBuf8<512> value;value.Copy(path);if(!ProjectPage(value))User::Leave(KErrPermissionDenied);
    CandidateVerifyL(fs,KPagesJournal);RFileReadStream in;User::LeaveIfError(in.Open(fs,KPagesJournal,EFileRead|EFileShareReadersOnly));CleanupClosePushL(in);
    if(in.ReadUint32L()!=KPagesJournalMagic)User::Leave(KErrCorrupt);TInt count=in.ReadInt32L();if(count<1||count>8)User::Leave(KErrNotSupported);
    TBuf8<32> saved;in>>saved;PageBackup pages[8];for(TInt i=0;i<count;i++){in>>pages[i].id;in>>pages[i].original;}CleanupStack::PopAndDestroy(&in);
    CHspsWrapper* hs=PagesClientLC();
    for(TInt i=0;i<count;i++){CHspsConfiguration* config=hs->GetPluginConfigurationL(pages[i].id);if(!config)User::Leave(KErrNotFound);CleanupStack::PushL(config);if(PagePathL(*config)->Value().CompareF(value))User::Leave(KErrInUse);CleanupStack::PopAndDestroy(config);}
    RAknsSrvSession skin;User::LeaveIfError(skin.Connect());CleanupClosePushL(skin);skin.RemoveWallpaper(path);CleanupStack::PopAndDestroy(&skin);
    Log(_L("RESUME refreshing session cache without activating any page; journal retained"));
    for(TInt i=0;i<count;i++)PageSetL(*hs,pages[i].id,value);
    User::LeaveIfError(AknsWallpaperUtils::SetIdleWallpaper(path,0));User::After(500000);
    CleanupStack::PopAndDestroy(hs);Log(_L("RESUME wallpaper refresh notification sent; no page activation"));
}
static void BindPagesL(const TDesC& path){
    TEntry entry;if(fs.Entry(KPagesJournal,entry)!=KErrNotFound)User::Leave(KErrInUse);
    CHspsWrapper* hs=PagesClientLC();CHspsConfiguration* app=hs->GetAppConfigurationL();if(!app)User::Leave(KErrNotFound);CleanupStack::PushL(app);
    TInt count=app->PluginMaps().Count();if(count<1||count>8)User::Leave(KErrNotSupported);PageBackup pages[8];
    for(TInt i=0;i<count;i++){
        pages[i].id.Copy(app->PluginMaps()[i]->PluginId());CHspsConfiguration* config=hs->GetPluginConfigurationL(pages[i].id);if(!config)User::Leave(KErrNotFound);CleanupStack::PushL(config);
        pages[i].original.Copy(PagePathL(*config)->Value());CleanupStack::PopAndDestroy(config);
        if(pages[i].original.Length()){
            if(!ProjectPage(pages[i].original))User::Leave(KErrNotSupported);
            // User confirmed every page should return to native black. Only
            // this project's stale experiment paths are normalized; log evidence.
            TBuf<512> old;old.Copy(pages[i].original);Log(_L("PAGES removing stale project reference on restore:"));Log(old);pages[i].original.Zero();
        }
    }
    TFileName temporary(KPagesJournal);temporary.Append(_L(".tmp"));
    RFileWriteStream out;User::LeaveIfError(out.Replace(fs,temporary,EFileWrite|EFileShareExclusive));CleanupClosePushL(out);
    out.WriteUint32L(KPagesJournalMagic);out.WriteInt32L(count);TBuf8<32> initial;PageActiveL(initial);out<<initial;for(TInt i=0;i<count;i++){out<<pages[i].id;out<<pages[i].original;}out.CommitL();out.Sink()->SynchL();
    CleanupStack::PopAndDestroy(&out);CandidateSealL(fs,temporary);User::LeaveIfError(fs.Rename(temporary,KPagesJournal));
    TBuf8<512> value;value.Copy(path);for(TInt i=0;i<count;i++){PageSetL(*hs,pages[i].id,value);PageActivateL(*hs,pages[i].id);User::LeaveIfError(AknsWallpaperUtils::SetIdleWallpaper(path,0));User::After(300000);TBuf<80> line;TBuf<32> id;id.Copy(pages[i].id);line.Format(_L("PAGES bound page id=%S"),&id);Log(line);}
    PageActivateL(*hs,initial);
    CleanupStack::PopAndDestroy(app);CleanupStack::PopAndDestroy(hs);
}
#endif
