#ifndef BELLE_BACKGROUND_INSPECT_H
#define BELLE_BACKGROUND_INSPECT_H
#include <eikenv.h>
#include "backgroundprofile.h"
// Invoked on the desktop UI thread. Uses the exported View() entry and a
// firmware-checked member offset, never casts a Window Server client handle.
// Resolves afresh on the UI thread. The caller may issue DrawDeferred only after validation.
static CCoeControl* InspectBackgroundL(TBool log=ETrue){
    if(RProcess().SecureId().iId!=0x102750f0){Trace(_L("BACKGROUND skip: not desktop"));return 0;}
    RLibrary library;User::LeaveIfError(library.Load(_L("xn3layoutengine.dll")));CleanupClosePushL(library);
    TLibraryFunction symbol=library.Lookup(237);if(!symbol)User::Leave(KErrNotSupported);
    const TUint base=(reinterpret_cast<TUint>(symbol)&~1u)-0x993c;
    if(!MatchBackgroundCode(base)){Trace(_L("BACKGROUND rejected: runtime code fingerprint"));User::Leave(KErrNotSupported);}
    typedef TAny* (*TViewFunction)(TAny*);
    TAny* view=reinterpret_cast<TViewFunction>(symbol)(CEikonEnv::Static()->EikAppUi());
    if(!view)User::Leave(KErrNotReady);
    CCoeControl* bg=*reinterpret_cast<CCoeControl**>(reinterpret_cast<TUint8*>(view)+0x5c);
    if(!bg||(reinterpret_cast<TUint>(bg)&3))User::Leave(KErrCorrupt);
    const TUint vtable=*reinterpret_cast<const TUint*>(bg);
    if(vtable!=base+0x40740||*reinterpret_cast<const TUint*>(vtable+0xa4)!=base+0x3231d){Trace(_L("BACKGROUND rejected: object vtable"));User::Leave(KErrNotSupported);}
    const TRect r=bg->Rect();TBuf<180> line;
    line.Format(_L("BACKGROUND verified own=%d visible=%d rect=%d,%d,%d,%d ws=%d"),bg->OwnsWindow(),bg->IsVisible(),r.iTl.iX,r.iTl.iY,r.Width(),r.Height(),bg->DrawableWindow()?bg->DrawableWindow()->WsHandle():0);if(log)Trace(line);
    CleanupStack::PopAndDestroy(&library);
    return bg;
}
#endif
