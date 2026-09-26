#ifndef BELLEWALL_WEBTRANSPORT_H
#define BELLEWALL_WEBTRANSPORT_H
#include <e32base.h>
#include "candidatesession.h"
_LIT(KWebChunk,"BelleWallWebFramesV3");
_LIT(KWebMutex,"BelleWallWebFramesLockV3");
const TInt KWebFrameBytes=180*320*2;
struct TWebFrames{TUint magic;TInt owner;TUint nonceLo;TUint nonceHi;TInt limitMs;TInt stop;TInt paused;TInt sequence;TInt slot;TInt renderUs;TInt copyUs;TUint producer;TUint8 pixels[2][180*320*2];};
const TUint KWebMagic=0x33574253;
#endif
