#ifndef BELLEWALL_RENDERODT_H
#define BELLEWALL_RENDERODT_H
#include "rendereridentity.h"
#include <xndomdocument.h>
#include <xndomnode.h>
#include <xndomattribute.h>
#include <xndomlist.h>
#include <xndomproperty.h>
#include <xndompropertyvalue.h>
#include <xnodt.h>
#include <hspsclient.h>
#include <hspsdefinitionrepository.h>
#include <hspsresource.h>
#include <plugininfo.h>
#include <hsccapiclient.h>
#include <hscontentcontrol.h>
#include <hscontentinfo.h>
#include <hscontentinfoarray.h>
static void RenderTextL(RFs& fs,const TDesC& name,const TDesC8& bytes){RFile file;User::LeaveIfError(file.Replace(fs,name,EFileWrite));CleanupClosePushL(file);RBuf8 formatted;formatted.CreateL(bytes.Length()*3+2);CleanupClosePushL(formatted);for(TInt i=0;i<bytes.Length();i++){formatted.Append(bytes[i]);if(bytes[i]=='>'&&i+1<bytes.Length()&&bytes[i+1]=='<')formatted.Append(_L8("\r\n"));}formatted.Append(_L8("\r\n"));User::LeaveIfError(file.Write(formatted));CleanupStack::PopAndDestroy(&formatted);User::LeaveIfError(file.Flush());CleanupStack::PopAndDestroy(&file);}
static void OdtIdL(CXnDomNode& node,const TDesC8& id){
    CXnDomAttribute* attribute=CXnDomAttribute::NewL(_L8("id"),node.StringPool());CleanupStack::PushL(attribute);attribute->SetValueL(id);node.AttributeList().AddItemL(attribute);CleanupStack::Pop(attribute);
}
static void OdtPixelsL(CXnDomNode& node,const TDesC8& name,TInt pixels){
    CXnDomProperty* property=CXnDomProperty::NewL(name,node.StringPool());CleanupStack::PushL(property);
    CXnDomPropertyValue* value=CXnDomPropertyValue::NewL(node.StringPool());CleanupStack::PushL(value);value->SetFloatValueL(CXnDomPropertyValue::EPx,pixels);
    property->PropertyValueList().AddItemL(value);CleanupStack::Pop(value);node.PropertyList().AddItemL(property);CleanupStack::Pop(property);
}
static void OdtNoneL(CXnDomNode& node,const TDesC8& name){
    CXnDomProperty* property=CXnDomProperty::NewL(name,node.StringPool());CleanupStack::PushL(property);
    CXnDomPropertyValue* value=CXnDomPropertyValue::NewL(node.StringPool());CleanupStack::PushL(value);value->SetStringValueL(CXnDomPropertyValue::EIdent,_L8("none"));
    property->PropertyValueList().AddItemL(value);CleanupStack::Pop(value);node.PropertyList().AddItemL(property);CleanupStack::Pop(property);
}
static void MakeRenderOdtL(){
    CXnDomDocument* doc=CXnDomDocument::NewL();CleanupStack::PushL(doc);
    _LIT8(ns,"http://www.series60.com/xml/xmluiml/1");
    CXnDomNode* root=doc->CreateElementNSL(_L8("xmluiml"),ns);doc->SetRootNode(root);
    CXnDomNode* widget=doc->CreateElementNSL(_L8("widget"),ns);CleanupStack::PushL(widget);root->AddChildL(widget);CleanupStack::Pop(widget);OdtIdL(*widget,_L8("bellewall_widget"));OdtPixelsL(*widget,_L8("width"),32);OdtPixelsL(*widget,_L8("height"),32);
    CXnDomNode* renderer=doc->CreateElementNSL(_L8(BW_RENDER_TYPE),ns);CleanupStack::PushL(renderer);widget->AddChildL(renderer);CleanupStack::Pop(renderer);OdtIdL(*renderer,_L8("bellewall_renderer"));OdtPixelsL(*renderer,_L8("width"),32);OdtPixelsL(*renderer,_L8("height"),32);
    OdtNoneL(*widget,_L8("background-color"));OdtNoneL(*widget,_L8("background-image"));
    OdtNoneL(*renderer,_L8("background-color"));OdtNoneL(*renderer,_L8("background-image"));
    RFs fs;User::LeaveIfError(fs.Connect());CleanupClosePushL(fs);TInt mkdir=fs.MkDirAll(_L("C:\\data\\BelleWall\\render-longrun-widget\\"));if(mkdir!=KErrAlreadyExists)User::LeaveIfError(mkdir);
    CDirectFileStore* store=CDirectFileStore::ReplaceLC(fs,_L("C:\\data\\BelleWall\\render-longrun-widget\\bellewall.o0000"),EFileWrite);store->SetTypeL(TUidType(KDirectFileStoreLayoutUid));RStoreWriteStream out;TStreamId rootStream=out.CreateLC(*store);
    // Legacy ODT header consumed by CXnODT, followed by an empty resource list
    // and the SDK's native DOM serialization. No ROM/private layout is patched.
    out.WriteUint32L(0x2001f48a);out.WriteUint32L(0x70031110);out.WriteUint32L(BW_RENDER_CONFIG_UID);
    out<<_L("BelleWall")<<_L("BelleWall longrun renderer")<<_L("BelleWall")<<_L("1.0");out.WriteInt32L(360);out.WriteInt32L(640);out.WriteInt32L(0);out.WriteInt32L(0);out.WriteInt16L(0);out.WriteInt32L(0);doc->ExternalizeL(out);out.CommitL();CleanupStack::PopAndDestroy(&out);store->SetRootL(rootStream);store->CommitL();CleanupStack::PopAndDestroy(store);
    CDirectFileStore* input=CDirectFileStore::OpenLC(fs,_L("C:\\data\\BelleWall\\render-longrun-widget\\bellewall.o0000"),EFileRead);RStoreReadStream in;in.OpenLC(*input,input->Root());CXnODT::InternalizeHeaderL(in);if(in.ReadInt32L()!=0)User::Leave(KErrCorrupt);CXnDomDocument* check=CXnDomDocument::NewL(in);CleanupStack::PushL(check);
    if(!check->RootNode()||check->RootNode()->Name()!=_L8("xmluiml")||check->DomNodeCount()!=3)User::Leave(KErrCorrupt);Trace(_L("ODT DirectFileStore/root/header/DOM round-trip verified; three nodes"));
    CleanupStack::PopAndDestroy(check);CleanupStack::PopAndDestroy(&in);CleanupStack::PopAndDestroy(input);
    RenderTextL(fs,_L("C:\\data\\BelleWall\\render-longrun-widget\\manifest.dat"),_L8("<?xml version=\"1.0\"?><package version=\"2.0\"><family>qhd_tch</family><type>widget</type><interfaceuid>0x2001f48a</interfaceuid><provideruid>0x70031110</provideruid><configurationuid>" BW_RENDER_CONFIG_TEXT "</configurationuid><fullname>BelleWall longrun renderer</fullname><shortname>BelleWall</shortname><version>1.0</version><filexml>widgetconfiguration.xml</filexml><localization><fileresource tag=\"xuikon\">bellewall.o0000</fileresource></localization></package>"));
    RenderTextL(fs,_L("C:\\data\\BelleWall\\render-longrun-widget\\widgetconfiguration.xml"),_L8("<configuration><control><settings/></control></configuration>"));
    CleanupStack::PopAndDestroy(&fs);CleanupStack::PopAndDestroy(doc);
}
class TRenderInstallObserver:public CTimer,public MhspsThemeManagementServiceObserver{
public:
    static TRenderInstallObserver* NewLC(){TRenderInstallObserver* self=new(ELeave)TRenderInstallObserver;CleanupStack::PushL(self);self->ConstructL();CActiveScheduler::Add(self);return self;}
    void Prepare(){iComplete=EFalse;iResult=EhspsServiceRequestSheduled;}
    TInt Await(TInt initial){if(initial!=EhspsServiceRequestSheduled)return initial;if(!iComplete){TTime now;now.UniversalTime();TInt64 remaining=iDeadline.MicroSecondsFrom(now).Int64();if(remaining<=0)return EhspsServiceRequestError;After(TTimeIntervalMicroSeconds32(TInt(remaining)));iWait.Start();Cancel();}return iResult;}
    void HandlehspsClientMessage(ThspsServiceCompletedMessage result){TBuf<80> line;line.Format(_L("RENDER INSTALL async result=%d"),result);Trace(line);if(result==EhspsServiceRequestSheduled||result==EhspsInstallPhaseSuccess)return;iResult=result;iComplete=ETrue;if(iWait.IsStarted())iWait.AsyncStop();}
private:
    TRenderInstallObserver():CTimer(EPriorityStandard),iComplete(EFalse),iResult(EhspsServiceRequestSheduled){iDeadline.UniversalTime();iDeadline+=TTimeIntervalSeconds(25);}
    void RunL(){Trace(_L("RENDER INSTALL timeout; cancel owned installation"));iResult=EhspsServiceRequestError;iComplete=ETrue;if(iWait.IsStarted())iWait.AsyncStop();}
    CActiveSchedulerWait iWait;TTime iDeadline;TBool iComplete;TInt iResult;
};
static void InstallRenderWidgetL(TBool legacy=EFalse){
    MakeRenderOdtL();if(legacy){RFs fs;User::LeaveIfError(fs.Connect());CleanupClosePushL(fs);RFile f;User::LeaveIfError(f.Open(fs,_L("C:\\data\\BelleWall\\render-longrun-widget\\manifest.dat"),EFileRead));CleanupClosePushL(f);TBuf8<2048> bytes;User::LeaveIfError(f.Read(bytes));CleanupStack::PopAndDestroy(&f);_LIT8(v2,"<package version=\"2.0\">");TInt at=bytes.Find(v2);if(at<0)User::Leave(KErrCorrupt);bytes.Replace(at,v2().Length(),_L8("<package version=\"1.0\">"));RenderTextL(fs,_L("C:\\data\\BelleWall\\render-longrun-widget\\manifest.dat"),bytes);CleanupStack::PopAndDestroy(&fs);Trace(_L("RENDER INSTALL version 1.0 manifest trial"));}RFs files;User::LeaveIfError(files.Connect());CleanupClosePushL(files);TInt mkdir=files.MkDirAll(_L("C:\\data\\BelleWall\\render-longrun-widget\\hsps\\00\\"));if(mkdir!=KErrAlreadyExists)User::LeaveIfError(mkdir);mkdir=files.MkDirAll(_L("C:\\data\\BelleWall\\render-longrun-widget\\xuikon\\00\\"));if(mkdir!=KErrAlreadyExists)User::LeaveIfError(mkdir);CFileMan* copy=CFileMan::NewL(files);CleanupStack::PushL(copy);User::LeaveIfError(copy->Copy(_L("C:\\data\\BelleWall\\render-longrun-widget\\manifest.dat"),_L("C:\\data\\BelleWall\\render-longrun-widget\\hsps\\00\\manifest.dat"),CFileMan::EOverWrite));User::LeaveIfError(copy->Copy(_L("C:\\data\\BelleWall\\render-longrun-widget\\widgetconfiguration.xml"),_L("C:\\data\\BelleWall\\render-longrun-widget\\hsps\\00\\widgetconfiguration.xml"),CFileMan::EOverWrite));User::LeaveIfError(copy->Copy(_L("C:\\data\\BelleWall\\render-longrun-widget\\bellewall.o0000"),_L("C:\\data\\BelleWall\\render-longrun-widget\\xuikon\\00\\bellewall.o0000"),CFileMan::EOverWrite));CleanupStack::PopAndDestroy(copy);CleanupStack::PopAndDestroy(&files);TRenderInstallObserver* observer=TRenderInstallObserver::NewLC();ChspsClient* client=ChspsClient::NewLC(*observer);ChspsODT* header=ChspsODT::NewL();CleanupStack::PushL(header);
    TInt result=client->hspsInstallTheme(_L("C:\\data\\BelleWall\\render-longrun-widget\\hsps\\00\\manifest.dat"),*header);TBuf<120> line;line.Format(_L("RENDER INSTALL initial=%d"),result);Trace(line);
    for(TInt phase=0;result==EhspsInstallPhaseSuccess&&phase<32;phase++){observer->Prepare();result=observer->Await(client->hspsInstallNextPhaseL(*header));line.Format(_L("RENDER INSTALL phase=%d result=%d"),phase,result);Trace(line);}
    ChspsResult* detail=ChspsResult::NewL();CleanupStack::PushL(detail);client->GethspsResult(*detail);line.Format(_L("RENDER INSTALL system=%d xuikon=%d d1=%d d2=%d"),detail->iSystemError,detail->iXuikonError,detail->iIntValue1,detail->iIntValue2);Trace(line);CleanupStack::PopAndDestroy(detail);
    if(result!=EhspsInstallThemeSuccess){client->hspsCancelInstallTheme();User::Leave(KErrGeneral);}
    if(header->ThemeUid()!=TInt(BW_RENDER_CONFIG_UID))User::Leave(KErrCorrupt);Trace(_L("RENDER INSTALL own widget registered; not attached to desktop"));CleanupStack::PopAndDestroy(header);CleanupStack::PopAndDestroy(client);CleanupStack::PopAndDestroy(observer);
}
class TRenderControlObserver:public MHsContentControl{public:void NotifyWidgetListChanged(){}void NotifyViewListChanged(){}void NotifyAppListChanged(){}};
static void RemoveRenderWidgetL(TUint configuration=BW_RENDER_CONFIG_UID){
    if(configuration!=BW_RENDER_CONFIG_UID&&configuration!=BelleRenderer::ConfigUid(1))User::Leave(KErrArgument);
    // Exact three-part identity created by this project; never an empty mask.
    TRenderInstallObserver* observer=TRenderInstallObserver::NewLC();
    ChspsClient* client=ChspsClient::NewLC(*observer);
    ChspsODT* mask=ChspsODT::NewL();CleanupStack::PushL(mask);
    mask->SetConfigurationType(EhspsWidgetConfiguration);mask->SetRootUid(0x2001f48a);mask->SetProviderUid(0x70031110);mask->SetThemeUid(configuration);
    TInt result=client->hspsRemoveThemeL(*mask);TBuf<100> line;line.Format(_L("RENDER REMOVE exact metadata 70031110/%08x result=%d"),configuration,result);Trace(line);
    if(result!=EhspsRemoveThemeSuccess)User::Leave(KErrGeneral);
    CleanupStack::PopAndDestroy(mask);CleanupStack::PopAndDestroy(client);CleanupStack::PopAndDestroy(observer);
}
#include "definitionprobe.h"
static void ClearPluginInfos(TAny* p){RPointerArray<CPluginInfo>* list=static_cast<RPointerArray<CPluginInfo>*>(p);list->ResetAndDestroy();list->Close();}
static void ListRenderWidgetL(){CHspsWrapper* hs=CHspsWrapper::NewLC(_L8("271012080"));RPointerArray<CPluginInfo> infos;CleanupStack::PushL(TCleanupItem(ClearPluginInfos,&infos));hs->GetPluginsL(infos,_L8("0x2001f48a"),_L8("widget"));TInt own=0;for(TInt j=0;j<infos.Count();j++)if(infos[j]->Uid().CompareF(_L8(BW_RENDER_CONFIG_TEXT))==0)own++;TBuf<100> direct;direct.Format(_L("RENDER direct HSPS widgets=%d own=%d"),infos.Count(),own);Trace(direct);CleanupStack::PopAndDestroy(&infos);CleanupStack::PopAndDestroy(hs);TRenderControlObserver observer;CHsCcApiClient* client=CHsCcApiClient::NewL(&observer);CleanupStack::PushL(client);MHsContentController* controller=client;CHsContentInfoArray* widgets=CHsContentInfoArray::NewL();CleanupStack::PushL(widgets);User::LeaveIfError(controller->WidgetListL(*widgets));TInt found=0;for(TInt i=0;i<widgets->Array().Count();i++)if(widgets->Array()[i]->Uid().CompareF(_L8(BW_RENDER_CONFIG_TEXT))==0)found++;TBuf<100> line;line.Format(_L("RENDER registry available=%d own_matches=%d"),widgets->Array().Count(),found);Trace(line);CleanupStack::PopAndDestroy(widgets);CleanupStack::PopAndDestroy(client);}
#endif
