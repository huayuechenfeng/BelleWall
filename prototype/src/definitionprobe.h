#ifndef BELLEWALL_DEFINITIONPROBE_H
#define BELLEWALL_DEFINITIONPROBE_H
#include <centralrepository.h>
#include <hspsodt.h>
#include <hspsthememanagement.h>
#include <hspsdefinitionrepository.h>
#include <hspsresource.h>
static void InspectRenderDefinitionL(){
    // FP2 hspsdefrep NewL invokes RProperty::Define/Set in category 200159c0.
    // It is a server-side constructor, NOT a read-only client diagnostic.
    // Keep the old command harmless; never instantiate it in a probe.
    Trace(_L("DEF disabled: constructor writes system notification properties"));
    User::Leave(KErrNotSupported);

}

#endif
