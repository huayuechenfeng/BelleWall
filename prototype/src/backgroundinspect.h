#ifndef BELLE_BACKGROUND_INSPECT_H
#define BELLE_BACKGROUND_INSPECT_H
#include <eikenv.h>
#include "backgroundprofiles.h"
// Invoked on the desktop UI thread. Uses the exported View() entry and a
// signature-checked member offset, never casts a Window Server client handle.
// Resolves afresh on the UI thread. The caller may issue DrawDeferred only after validation.
static CCoeControl* InspectBackgroundL(TBool log=ETrue){
    if(RProcess().SecureId().iId!=0x102750f0){Trace(_L("BACKGROUND skip: not desktop"));return 0;}
    RLibrary library;User::LeaveIfError(library.Load(_L("xn3layoutengine.dll")));CleanupClosePushL(library);
    TLibraryFunction symbol=library.Lookup(237);if(!symbol)User::Leave(KErrNotSupported);
    TUint base=0;TBackgroundLayout layout;
    if(!FindBackgroundLayout(library,symbol,base,layout)){Trace(_L("BACKGROUND rejected: runtime code fingerprint"));User::Leave(KErrNotSupported);}
    typedef TAny* (*TViewFunction)(TAny*);
    TAny* view=reinterpret_cast<TViewFunction>(symbol)(CEikonEnv::Static()->EikAppUi());
    if(!view)User::Leave(KErrNotReady);
    if(reinterpret_cast<TUint>(view)&3)User::Leave(KErrCorrupt);
    CCoeControl* bg=*reinterpret_cast<CCoeControl**>(reinterpret_cast<TUint8*>(view)+layout.backgroundOffset);
    if(!bg||(reinterpret_cast<TUint>(bg)&3))User::Leave(KErrCorrupt);
    const TUint vtable=*reinterpret_cast<const TUint*>(bg);
    if(vtable!=base+layout.vtableAddress||*reinterpret_cast<const TUint*>(vtable+0xa4)!=base+layout.drawAddress){Trace(_L("BACKGROUND rejected: object vtable"));User::Leave(KErrNotSupported);}
    const TRect r=bg->Rect();TBuf<180> line;
    line.Format(_L("BACKGROUND verified layout=%d own=%d visible=%d rect=%d,%d,%d,%d ws=%d"),layout.id,bg->OwnsWindow(),bg->IsVisible(),r.iTl.iX,r.iTl.iY,r.Width(),r.Height(),bg->DrawableWindow()?bg->DrawableWindow()->WsHandle():0);if(log)Trace(line);
    CleanupStack::PopAndDestroy(&library);
    return bg;
}
#endif
