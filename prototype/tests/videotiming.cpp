#include "../src/videotiming.h"
#include <cassert>
int main(){
    using namespace BelleVideoTiming;
    assert(NextDueUs(0,10,1)==100000);
    assert(NextDueUs(0,20,1)==50000);
    assert(NextDueUs(0,30,1)==33334);
    assert(NextDueUs(33334,30,1)==66667);
    assert(SourceFrame(33334,30,1,300)==1);
    assert(SourceFrame(33334,20,1,300)==0);
    assert(NextDueUs(0,60,1)==33334);
    assert(SourceFrame(33334,60,1,300)==2);
    assert(NextDueUs(0,30000,1001)==33367);
    assert(SourceFrame(33367,30000,1001,300)==1);
    assert(SourceFrame(1000000,30,1,30)==0);
    assert(NextDueUs(1000000,30,1)>1000000);
    assert(NextDueUs(721234567,30,1)>721234567);
}
