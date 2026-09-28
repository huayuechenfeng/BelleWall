#ifndef BELLEWALL_WEBTRANSPORT_H
#define BELLEWALL_WEBTRANSPORT_H
#include <e32base.h>
#include "candidatesession.h"
_LIT(KWebChunk,"BelleWallWebFramesV4");
_LIT(KWebMutex,"BelleWallWebFramesLockV4");
const TInt KWebFrameBytes=180*320*2;
const TInt KWebCapacity=1048576*2;
struct TWebFrames{TUint magic;TInt owner;TUint nonceLo;TUint nonceHi;TInt limitMs;TInt stop;TInt paused;TInt sequence;TInt slot;TInt renderUs;TInt copyUs;TUint producer;TInt width,height,requestedWidth,requestedHeight;TUint generation,requestedGeneration;TUint8 pixels[2][KWebCapacity];};
const TUint KWebMagic=0x34574253;
#endif
