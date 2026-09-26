// Exercises our own renderer in an invisible control, never in the homescreen.
#include <e32base.h>
#include <f32file.h>
#include <coemain.h>
#include <eikenv.h>
#include <coecntrl.h>
#include <ecom/ecom.h>
#include <ecom/implementationinformation.h>
#include <xnextrenderingpluginadapter.h>
#include "renderlog.h"
static void Trace(const TDesC& event){BoundedRenderTrace(_L("C:\\data\\BelleWall\\render-plugin.log"),_L("C:\\data\\BelleWall\\render-plugin.previous.log"),event);}
#include "hsinspect.h"
#include "renderodt.h"
#include "renderattach.h"
static void FreeEntries(TAny* ptr);
#include "renderprepare.h"
#include "renderregistration.h"
class CProbeParent:public CCoeControl{public:void InitL(){CreateWindowL();MakeVisible(EFalse);}TInt WindowId()const{return Window().WsHandle();}};
static void FreeEntries(TAny* ptr){RImplInfoPtrArray* entries=static_cast<RImplInfoPtrArray*>(ptr);entries->ResetAndDestroy();entries->Close();}
static void TestL(){
    Trace(_L("HOST begin; invisible own window; no desktop configuration writes"));
    RImplInfoPtrArray entries;CleanupStack::PushL(TCleanupItem(FreeEntries,&entries));REComSession::ListImplementationsL(TUid::Uid(0x200286df),entries);TBool found=EFalse;
    for(TInt i=0;i<entries.Count();i++)if(entries[i]->ImplementationUid().iUid==TInt(BW_RENDER_IMPL_UID)&&entries[i]->DataType()==_L8(BW_RENDER_TYPE))found=ETrue;
    CleanupStack::PopAndDestroy(&entries);if(!found)User::Leave(KErrNotFound);Trace(_L("HOST exact ECom UID and data type found"));
    CProbeParent* parent=new(ELeave)CProbeParent;CleanupStack::PushL(parent);parent->InitL();TBuf<80> parentInfo;parentInfo.Format(_L("HOST parent window=%d"),parent->WindowId());Trace(parentInfo);
    CXnExtRenderingPluginAdapter* plugin=CXnExtRenderingPluginAdapter::NewL(TUid::Uid(BW_RENDER_IMPL_UID));CleanupStack::PushL(plugin);
    plugin->SetContainerWindowL(*parent);plugin->SetRect(TRect(0,0,16,16));
    Trace(_L("HOST compare parent window with plugin log"));plugin->EnterPowerSaveModeL();plugin->ExitPowerSaveModeL();
    CleanupStack::PopAndDestroy(plugin);CleanupStack::PopAndDestroy(parent);Trace(_L("HOST lifecycle test completed"));
}
GLDEF_C TInt E32Main(){CTrapCleanup* cleanup=CTrapCleanup::New();if(!cleanup)return KErrNoMemory;CEikonEnv* env=new CEikonEnv;if(!env){delete cleanup;return KErrNoMemory;}TBuf<64> args;User::CommandLine(args);TRAPD(error,env->ConstructL(EFalse);if(args==_L("--cleanup-wallpaper")){ManageRenderRegistrationL(ETrue,EFalse,ETrue);RProcess::Rendezvous(BellePreparation::CleanupProtocol);}else if(args==_L("--prepare-wallpaper")){ManageRenderRegistrationL(EFalse,ETrue);RProcess::Rendezvous(BellePreparation::Protocol);}else if(args.Find(_L("--candidate-"))==0){CandidateWidgetL(args);}else if(args.Find(_L("--guard-render-widget"))!=KErrNotFound){GuardRenderWidgetL(args.Find(_L("--guard-render-widget-manual"))!=KErrNotFound,args.Find(_L("--guard-render-widget-pages"))!=KErrNotFound);}else if(args.Find(_L("--attach-render-pages"))!=KErrNotFound){AttachRenderWidgetL(ETrue);}else if(args.Find(_L("--attach-render-widget"))!=KErrNotFound){AttachRenderWidgetL();}else if(args.Find(_L("--detach-render-widget"))!=KErrNotFound){DetachOwnRenderL();}else if(args.Find(_L("--remove-render-widget"))!=KErrNotFound){ManageRenderRegistrationL(ETrue);}else if(args.Find(_L("--inspect-render-definition"))!=KErrNotFound){InspectRenderDefinitionL();}else if(args.Find(_L("--list-render-widget"))!=KErrNotFound){ListRenderWidgetL();}else if(args.Find(_L("--install-render-widget"))!=KErrNotFound){if(args.Find(_L("--install-render-widget-v1"))!=KErrNotFound)User::Leave(KErrNotSupported);ManageRenderRegistrationL(EFalse);}else if(args.Find(_L("--make-render-odt"))!=KErrNotFound){MakeRenderOdtL();}else if(args.Find(_L("--hs-inspect"))!=KErrNotFound){TRAPD(direct,HsDirectL());TBuf<80> detail;detail.Format(_L("HS DIRECT trapped=%d"),direct);Trace(detail);HsInspectL();}else TestL());REComSession::FinalClose();TBuf<80> line;line.Format(_L("HOST result=%d"),error);Trace(line);env->DestroyEnvironment();delete cleanup;return error;}
