#ifndef BELLEWALL_RENDER_PREPARE_H
#define BELLEWALL_RENDER_PREPARE_H
#include <bafl/sysutil.h>
#include <AknSkinsInternalCRKeys.h>
#include "backgroundprofile.h"
#include "preparationpolicy.h"
// Called inside BOTH registration locks, before any registration writes.
static void PrepareEnvironmentL(RFs& files){
    TBuf<KSysUtilVersionTextLength> version;User::LeaveIfError(SysUtil::GetSWVersion(version));
    if(version.Find(_L("113.010.1506"))!=0||version.Find(_L("RM-779"))<0)User::Leave(BellePreparation::Environment);
    const TSize size=CEikonEnv::Static()->ScreenDevice()->SizeInPixels();
    if(size!=TSize(360,640))User::Leave(BellePreparation::Environment);
    RLibrary library;User::LeaveIfError(library.Load(_L("Z:\\sys\\bin\\xn3layoutengine.dll")));CleanupClosePushL(library);
    TLibraryFunction symbol=library.Lookup(237);if(!symbol)User::Leave(BellePreparation::Environment);
    const TUint base=(reinterpret_cast<TUint>(symbol)&~1u)-0x993c;
    if(!MatchBackgroundCode(base))User::Leave(BellePreparation::Environment);
    CleanupStack::PopAndDestroy(&library);Trace(_L("PREPARE firmware, portrait size and native code fingerprint verified"));
    RImplInfoPtrArray entries;CleanupStack::PushL(TCleanupItem(FreeEntries,&entries));
    REComSession::ListImplementationsL(TUid::Uid(0x200286df),entries);TInt found=0;
    for(TInt i=0;i<entries.Count();i++)if(entries[i]->ImplementationUid().iUid==TInt(BW_RENDER_IMPL_UID)&&entries[i]->DataType()==_L8(BW_RENDER_TYPE))found++;
    CleanupStack::PopAndDestroy(&entries);if(found!=1)User::Leave(BellePreparation::Components);
    CRepository* repo=CRepository::NewLC(KCRUidPersonalisation);TFileName path;TInt kind=-1;
    User::LeaveIfError(repo->Get(KPslnIdleBackgroundImagePath,path));User::LeaveIfError(repo->Get(KPslnWallpaperType,kind));
    CleanupStack::PopAndDestroy(repo);if(path.Length()||kind!=0)User::Leave(BellePreparation::Background);
    CHspsWrapper* hs=CHspsWrapper::NewLC(_L8("271012080"));CHspsConfiguration* app=hs->GetAppConfigurationL();if(!app)User::Leave(KErrNotFound);CleanupStack::PushL(app);
    if(app->PluginMaps().Count()!=4)User::Leave(BellePreparation::Background);
    for(TInt i=0;i<4;i++){
        CHspsConfiguration* page=hs->GetPluginConfigurationL(app->PluginMaps()[i]->PluginId());if(!page)User::Leave(KErrNotFound);CleanupStack::PushL(page);
        TBool foundPath=EFalse;RPointerArray<CItemMap>& items=page->Settings();
        for(TInt j=0;j<items.Count();j++)if(items[j]->ItemId()==_L8("wallpaper")){
            RPointerArray<CPropertyMap>& props=items[j]->Properties();
            for(TInt k=0;k<props.Count();k++)if(props[k]->Name()==_L8("path")){if(foundPath||props[k]->Value().Length())User::Leave(BellePreparation::Background);foundPath=ETrue;}
        }
        if(!foundPath)User::Leave(BellePreparation::Background);CleanupStack::PopAndDestroy(page);
    }
    CleanupStack::PopAndDestroy(app);CleanupStack::PopAndDestroy(hs);
    TVolumeInfo volume;User::LeaveIfError(files.Volume(volume,EDriveC));if(volume.iFree<8*1024*1024)User::Leave(KErrDiskFull);
    Trace(_L("PREPARE exact ECom identity, four default pages and C drive reserve verified"));
}
#endif
