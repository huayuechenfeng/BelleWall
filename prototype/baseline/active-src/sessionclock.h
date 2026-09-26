#ifndef BELLEWALL_SESSION_CLOCK_H
#define BELLEWALL_SESSION_CLOCK_H
#include <hal.h>
// Sample at least once per 32-bit nano-tick wrap. Unlike FastCounter deltas,
// this clock is suitable for long gaps and accumulates in 64 bits.
class SessionClock {
public:
    void InitL(){User::LeaveIfError(HAL::Get(HALData::ENanoTickPeriod,period));if(period<=0)User::Leave(KErrNotSupported);last=User::NTickCount();}
    TInt64 StepUs(){TUint now=User::NTickCount();TUint ticks=now-last;last=now;return TInt64(ticks)*period;}
private:TUint last;TInt period;
};
#endif
