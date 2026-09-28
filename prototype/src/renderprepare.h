#ifndef BELLEWALL_RENDER_PREPARE_H
#define BELLEWALL_RENDER_PREPARE_H
#include <AknSkinsInternalCRKeys.h>
#include "backgroundprofiles.h"
#include "preparationpolicy.h"
#include "displaypolicy.h"
// Called inside BOTH registration locks, before any registration writes.
static void PrepareEnvironmentL(RFs& files){
    const TSize size=CEikonEnv::Static()->ScreenDevice()->SizeInPixels();
    if(!BelleDisplay::Valid(size.iWidth,size.iHeight))User::Leave(BellePreparation::Environment);
    RLibrary library;User::LeaveIfError(library.Load(_L("xn3layoutengine.dll")));CleanupClosePushL(library);
    TLibraryFunction symbol=library.Lookup(237);if(!symbol)User::Leave(BellePreparation::Environment);
    TUint base=0;TBackgroundLayout layout;
    if(!FindBackgroundLayout(library,symbol,base,layout))User::Leave(BellePreparation::Environment);
    TBuf<96> selected;if(layout.id==0)selected.Copy(_L("PREPARE generic structural layout and screen size verified"));
    else selected.Format(_L("PREPARE native layout=%d screen size and code fingerprint verified"),layout.id);
    CleanupStack::PopAndDestroy(&library);Trace(selected);
    RImplInfoPtrArray entries;CleanupStack::PushL(TCleanupItem(FreeEntries,&entries));
    REComSession::ListImplementationsL(TUid::Uid(0x200286df),entries);TInt found=0;
    for(TInt i=0;i<entries.Count();i++)if(entries[i]->ImplementationUid().iUid==TInt(BW_RENDER_IMPL_UID)&&entries[i]->DataType()==_L8(BW_RENDER_TYPE))found++;
    CleanupStack::PopAndDestroy(&entries);if(found!=1)User::Leave(BellePreparation::Components);
    CRepository* repo=CRepository::NewLC(KCRUidPersonalisation);TFileName path;TInt kind=-1;
    User::LeaveIfError(repo->Get(KPslnIdleBackgroundImagePath,path));User::LeaveIfError(repo->Get(KPslnWallpaperType,kind));
    CleanupStack::PopAndDestroy(repo);if(path.Length()||kind!=0)User::Leave(BellePreparation::Background);
    CHspsWrapper* hs=CHspsWrapper::NewLC(_L8("271012080"));CHspsConfiguration* app=hs->GetAppConfigurationL();if(!app)User::Leave(KErrNotFound);CleanupStack::PushL(app);
    const TInt pageCount=app->PluginMaps().Count();if(pageCount<1||pageCount>8)User::Leave(BellePreparation::Background);
    for(TInt i=0;i<pageCount;i++){
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
    TBuf<96> ready;ready.Format(_L("PREPARE ECom identity, default pages=%d and C drive reserve verified"),pageCount);Trace(ready);
}
#endif
