#ifndef BELLEWALL_RENDER_REGISTRATION_H
#define BELLEWALL_RENDER_REGISTRATION_H
#include "preparationtransaction.h"
// Application preparation and explicit install/remove share the same guards.
_LIT(KPreparationJournal,"C:\\data\\BelleWall\\preparation-pending.bin");
static void ReadPreparationJournalL(RFs& files){
    TEntry entry;TInt exists=files.Entry(KPreparationJournal,entry);if(exists==KErrNotFound)return;User::LeaveIfError(exists);
    BellePreparation::Journal record;RFile file;User::LeaveIfError(file.Open(files,KPreparationJournal,EFileRead|EFileShareReadersOnly));CleanupClosePushL(file);
    TInt size;User::LeaveIfError(file.Size(size));TPtr8 bytes(reinterpret_cast<TUint8*>(&record),0,sizeof(record));User::LeaveIfError(file.Read(bytes));
    if(size!=sizeof(record)||bytes.Length()!=sizeof(record)||!BellePreparation::Valid(record))User::Leave(BellePreparation::JournalDamaged);
    CleanupStack::PopAndDestroy(&file);Trace(_L("PREPARE pending registration transaction; re-querying actual state"));
}
static void WritePreparationJournalL(RFs& files,TUint operation){
    BellePreparation::Journal record={BellePreparation::Protocol,BW_RENDER_CONFIG_UID,operation,0};record.check=BellePreparation::Seal(record);
    TFileName temp(KPreparationJournal);temp.Append(_L(".tmp"));RFile file;User::LeaveIfError(file.Replace(files,temp,EFileWrite|EFileShareExclusive));CleanupClosePushL(file);
    User::LeaveIfError(file.Write(TPtrC8(reinterpret_cast<const TUint8*>(&record),sizeof(record))));User::LeaveIfError(file.Flush());CleanupStack::PopAndDestroy(&file);User::LeaveIfError(files.Replace(temp,KPreparationJournal));
}
static TInt RegisteredLongrunCountL(TUint version=2){
    if(!BelleRenderer::KnownVersion(version))User::Leave(KErrArgument);
    const TUint configuration=BelleRenderer::ConfigUid(version);TBuf8<16> configText;configText.Format(_L8("0x%08x"),configuration);
    CHspsWrapper* hs=CHspsWrapper::NewLC(_L8("271012080"));RPointerArray<CPluginInfo> infos;CleanupStack::PushL(TCleanupItem(ClearPluginInfos,&infos));
    hs->GetPluginsL(infos,_L8("0x2001f48a"),_L8("widget"));TInt count=0;for(TInt i=0;i<infos.Count();i++)if(infos[i]->Uid().CompareF(configText)==0)count++;
    CleanupStack::PopAndDestroy(&infos);CleanupStack::PopAndDestroy(hs);TBuf<180> detail;detail.Format(_L("REGISTRATION config=%08x content definitions matching=%d"),configuration,count);Trace(detail);
    if(count>1)User::Leave(KErrCorrupt);if(!count)return 0;
    TRenderInstallObserver* observer=TRenderInstallObserver::NewLC();ChspsClient* client=ChspsClient::NewLC(*observer);
    ChspsODT* header=ChspsODT::NewL();CleanupStack::PushL(header);
    const TInt result=client->hspsGetPluginOdtL(0x102750f0,configuration,header);
    detail.Format(_L("REGISTRATION plugin ODT result=%d root=%08x provider=%08x config=%08x type=%u"),result,header->RootUid(),header->ProviderUid(),header->ThemeUid(),header->ConfigurationType());Trace(detail);
    if(result!=EhspsGetPluginOdtSuccess)User::Leave(KErrNotReady);
    if(header->RootUid()!=TInt(0x2001f48a)||header->ProviderUid()!=TInt(0x70031110)||header->ThemeUid()!=TInt(configuration)||header->ConfigurationType()!=EhspsWidgetConfiguration)User::Leave(KErrPermissionDenied);
    CleanupStack::PopAndDestroy(header);CleanupStack::PopAndDestroy(client);CleanupStack::PopAndDestroy(observer);return count;
}
class TPreparationBackend {
public:
    explicit TPreparationBackend(RFs& fs):files(fs){}
    TInt Count(TUint version){return RegisteredLongrunCountL(version);}
    void Begin(TUint operation){WritePreparationJournalL(files,operation);}
    void Install(){InstallRenderWidgetL();}
    void Remove(TUint version){RemoveRenderWidgetL(BelleRenderer::ConfigUid(version));}
    void Fail(){User::Leave(KErrCorrupt);}
    void Commit(){User::LeaveIfError(files.Delete(KPreparationJournal));Trace(_L("PREPARE transaction committed; journal cleared"));}
private:RFs& files;
};
static void ManageRenderRegistrationL(TBool remove,TBool prepare=EFalse,TBool fullCleanup=EFalse){
    RMutex wallpaper;User::LeaveIfError(wallpaper.CreateGlobal(_L("BelleWallWallpaperExperiment")));CleanupClosePushL(wallpaper);
    RMutex widget;User::LeaveIfError(widget.CreateGlobal(_L("BelleWallCandidateWidgetOperation")));CleanupClosePushL(widget);
    RFs files;User::LeaveIfError(files.Connect());CleanupClosePushL(files);TEntry entry;
    if(files.Entry(KCandidateJournal,entry)!=KErrNotFound||files.Entry(_L("C:\\data\\BelleWall\\wallpaper-rollback.bin"),entry)!=KErrNotFound||files.Entry(_L("C:\\data\\BelleWall\\pages-rollback.bin"),entry)!=KErrNotFound)User::Leave(KErrInUse);
    renderSessionVersion=1;if(ScanOwnRenderL(EFalse))User::Leave(KErrInUse);
    renderSessionVersion=BW_RENDER_SESSION_VERSION;if(ScanOwnRenderL(EFalse))User::Leave(KErrInUse);
    if(prepare)PrepareEnvironmentL(files);
    ReadPreparationJournalL(files);TPreparationBackend backend(files);
    BellePreparation::Transaction(backend,fullCleanup?3u:remove?2u:1u);
    Trace(fullCleanup?_L("PREPARE uninstall cleanup verified; current and legacy registrations absent"):_L("PREPARE registration operation verified"));
    CleanupStack::PopAndDestroy(&files);CleanupStack::PopAndDestroy(&widget);CleanupStack::PopAndDestroy(&wallpaper);
}
#endif
