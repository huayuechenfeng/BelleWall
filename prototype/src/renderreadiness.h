#ifndef BELLEWALL_RENDER_READINESS_H
#define BELLEWALL_RENDER_READINESS_H
namespace BelleRenderReadiness {
// Session heartbeats only prove that some plugin is running. Require an
// explicit acknowledgement through the current redraw transport as well.
const unsigned Protocol=0x00030001u;
const int Unavailable=-7110;
class Handshake {
public:
    enum Result { Pending, Ready, Failed };
    Handshake():foregroundUs(0){}
    Result Observe(unsigned protocol,long long deltaUs,bool foreground){
        if(protocol==Protocol)return Ready;
        if(!foreground||deltaUs<0||deltaUs>5000000){foregroundUs=0;return Pending;}
        foregroundUs+=deltaUs;
        return foregroundUs>=5000000?Failed:Pending;
    }
private:long long foregroundUs;
};
}
#endif
