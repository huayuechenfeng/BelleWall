#ifndef BELLEWALL_HSINSPECT_H
#define BELLEWALL_HSINSPECT_H
#include <hspswrapper.h>
#include <hspsconfiguration.h>
#include <pluginmap.h>
#include <itemmap.h>
#include <propertymap.h>
#include <liwservicehandler.h>
#include <liwvariant.h>
#include <centralrepository.h>
#include <activeidle2domaincrkeys.h>
#include <hspsrequestclient.h>
#include <hspsodt.h>
#include <hspsresult.h>
#include <s32file.h>
using namespace hspswrapper;
class THsObserver:public MhspsClientRequestServiceObserver{public:void HandlehspsRequestClientMessageL(ThspsServiceCompletedMessage,ChspsRequestNotificationParams&){Trace(_L("HS DIRECT unexpected notification"));}};
static void HsDirectL(){THsObserver observer;Trace(_L("HS DIRECT creating request client"));ChspsRequestClient* client=ChspsRequestClient::NewLC(observer);ChspsODT* odt=ChspsODT::NewL();CleanupStack::PushL(odt);TInt status=client->hspsGetODT(0x102750f0,*odt);TBuf<160> line;line.Format(_L("HS DIRECT status=%d root=%08x theme=%08x resources=%d"),status,odt->RootUid(),odt->ThemeUid(),odt->ResourceCount());Trace(line);ChspsResult* detail=ChspsResult::NewL();CleanupStack::PushL(detail);client->GethspsResult(*detail);line.Format(_L("HS DIRECT system=%d xuikon=%d detail1=%d detail2=%d"),detail->iSystemError,detail->iXuikonError,detail->iIntValue1,detail->iIntValue2);Trace(line);CleanupStack::PopAndDestroy(detail);if(status!=EhspsGetODTSuccess)User::Leave(KErrNotFound);RFs fs;User::LeaveIfError(fs.Connect());CleanupClosePushL(fs);RFileWriteStream stream;User::LeaveIfError(stream.Replace(fs,_L("C:\\data\\BelleWall\\hs-active.odt"),EFileWrite));CleanupClosePushL(stream);odt->ExternalizeL(stream);stream.CommitL();CleanupStack::PopAndDestroy(&stream);CleanupStack::PopAndDestroy(&fs);CleanupStack::PopAndDestroy(odt);CleanupStack::PopAndDestroy(client);Trace(_L("HS DIRECT read-only ODT saved"));}
static void HsServiceCheckL(){
    Trace(_L("HS CHECK opening repository"));CRepository* repo=CRepository::NewLC(TUid::Uid(KCRUidActiveIdleLV));TBuf8<32> id;TInt result=repo->Get(KAIActiveViewPluginId,id);TBuf<120> line;TBuf<32> id16;id16.Copy(id);line.Format(_L("HS CHECK active id result=%d value=%S"),result,&id16);Trace(line);CleanupStack::PopAndDestroy(repo);
    Trace(_L("HS CHECK creating LIW"));CLiwServiceHandler* handler=CLiwServiceHandler::NewL();CleanupStack::PushL(handler);
    CLiwCriteriaItem* criteria=CLiwCriteriaItem::NewL(1,_L8("IConfiguration"),_L8("Service.HSPS"));CleanupStack::PushL(criteria);criteria->SetServiceClass(TUid::Uid(KLiwClassBase));RCriteriaArray interest;CleanupClosePushL(interest);interest.AppendL(criteria);
    Trace(_L("HS CHECK attaching LIW"));handler->AttachL(interest);Trace(_L("HS CHECK attached"));
    CLiwGenericParamList& in=handler->InParamListL();CLiwGenericParamList& out=handler->OutParamListL();TBuf8<20> app;app.Num(0x102750f0);TLiwVariant v;v.Set(app);TLiwGenericParam param;param.SetNameAndValueL(_L8("appUid"),v);param.PushL();in.AppendL(param);CleanupStack::Pop();param.Reset();
    Trace(_L("HS CHECK executing service attach"));handler->ExecuteServiceCmdL(*criteria,in,out);line.Format(_L("HS CHECK output count=%d"),out.Count());Trace(line);
    for(TInt i=0;i<out.Count();i++){TBuf<80> name;name.Copy(out[i].Name().Left(80));Trace(name);}
    in.Reset();out.Reset();CleanupStack::PopAndDestroy(&interest);CleanupStack::PopAndDestroy(criteria);CleanupStack::PopAndDestroy(handler);
}
static void HsConfigL(CHspsWrapper& hs,CHspsConfiguration& config,TInt depth){
    TBuf<180> line;TBuf<60> id;id.Copy(config.ConfId().Left(60));line.Format(_L("HS CONFIG depth=%d id=%S children=%d"),depth,&id,config.PluginMaps().Count());Trace(line);
    RPointerArray<CItemMap>& settings=config.Settings();
    for(TInt i=0;i<settings.Count();i++){CItemMap& item=*settings[i];if(item.ItemId()!=_L8("wallpaper"))continue;RPointerArray<CPropertyMap>& props=item.Properties();for(TInt j=0;j<props.Count();j++){TBuf<50> key;key.Copy(props[j]->Name().Left(50));TBuf<100> value;value.Copy(props[j]->Value().Left(100));line.Format(_L("HS WALLPAPER %S=%S"),&key,&value);Trace(line);}}
    if(depth>=2)return;
    for(TInt i=0;i<config.PluginMaps().Count();i++){CPluginMap& map=*config.PluginMaps()[i];TBuf<60> child,uid;child.Copy(map.PluginId().Left(60));uid.Copy(map.PluginUid().Left(60));line.Format(_L("HS CHILD id=%S uid=%S active=%d"),&child,&uid,map.ActivationState());Trace(line);CHspsConfiguration* next=hs.GetPluginConfigurationL(map.PluginId());if(next){CleanupStack::PushL(next);HsConfigL(hs,*next,depth+1);CleanupStack::PopAndDestroy(next);}}
}
static void HsInspectL(){TBuf8<20> app;app.Num(0x102750f0);Trace(_L("HS opening wrapper"));CHspsWrapper* hs=CHspsWrapper::NewLC(app);Trace(_L("HS wrapper opened; reading active app"));CHspsConfiguration* config=hs->GetAppConfigurationL();if(!config){Trace(_L("HS active app result was null"));User::Leave(KErrNotFound);}CleanupStack::PushL(config);HsConfigL(*hs,*config,0);CleanupStack::PopAndDestroy(config);CleanupStack::PopAndDestroy(hs);Trace(_L("HS inspection complete; no setters called"));}
#endif
