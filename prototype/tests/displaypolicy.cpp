#include "../src/displaypolicy.h"
#include "../src/renderreadiness.h"
#include <cassert>
using namespace BelleDisplay;
int main(){
    assert(Valid(640,480)&&Valid(360,640)&&!Valid(0,640)&&!Valid(2048,2048));
    Mapping cover=Map(360,640,640,360,Cover);assert(cover.sx==0&&cover.sw==360&&cover.sh==202&&cover.sy==219&&cover.dw==640&&cover.dh==360);
    Mapping contain=Map(360,640,640,360,Contain);assert(contain.dw==202&&contain.dh==360&&contain.dx==219&&contain.dy==0);
    Mapping e6=Map(360,640,640,480,Contain);assert(e6.dw==270&&e6.dx==185&&e6.dh==480);
    Mapping stretch=Map(360,640,640,480,Stretch);assert(stretch.sw==360&&stretch.sh==640&&stretch.dw==640&&stretch.dh==480);
    for(int fit=0;fit<3;fit++)for(int sw=2;sw<800;sw+=37)for(int sh=2;sh<800;sh+=71){Mapping r=Map(sw,sh,640,480,fit);assert(r.sw>0&&r.sh>0&&r.dw>0&&r.dh>0&&r.sx>=0&&r.sy>=0&&r.dx>=0&&r.dy>=0&&r.sx+r.sw<=sw&&r.sy+r.sh<=sh&&r.dx+r.dw<=640&&r.dy+r.dh<=480);}
    assert(Allowed(640,480,Auto)&&Allowed(640,480,Landscape)&&!Allowed(640,480,Portrait));
    SettledSize s;assert(!s.Observe(360,640));assert(!s.Observe(640,360));assert(s.Observe(640,360));assert(!s.Observe(0,0));assert(!s.Observe(360,640));assert(s.Observe(360,640));
    s.Reset();assert(!s.Observe(360,640));assert(s.Observe(360,640));
    assert(CurrentGeometry(0,0,360,640,360,640));
    assert(!CurrentGeometry(1,0,640,360,360,640));
    assert(!CurrentGeometry(1,0,360,640,360,640)); // same-sized cache recreation
    assert(!CurrentGeometry(2,1,640,360,640,360)); // layout changed during rebuild
    assert(CurrentGeometry(2,2,640,360,640,360));
    using BelleRenderReadiness::Handshake;
    Handshake missing;
    for(int n=0;n<49;n++)assert(missing.Observe(0,100000,true)==Handshake::Pending);
    assert(missing.Observe(0,100000,true)==Handshake::Failed); // old DLL can heartbeat forever
    Handshake current;assert(current.Observe(BelleRenderReadiness::Protocol,100000,true)==Handshake::Ready);
    Handshake paused;assert(paused.Observe(0,4900000,true)==Handshake::Pending);
    assert(paused.Observe(0,100000,false)==Handshake::Pending);
    assert(paused.Observe(0,100000,true)==Handshake::Pending);
    assert(paused.Observe(0,10000000,true)==Handshake::Pending); // suspend is not a protocol timeout
    assert(paused.Observe(2,100000,true)==Handshake::Pending); // other protocol is not acceptance
    assert(paused.Observe(BelleRenderReadiness::Protocol,100000,true)==Handshake::Ready);
    return 0;
}
