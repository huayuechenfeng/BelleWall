#ifndef BELLEWALL_DISPLAY_POLICY_H
#define BELLEWALL_DISPLAY_POLICY_H
namespace BelleDisplay {
enum Fit { Cover=0, Contain=1, Stretch=2 };
enum Orientation { Auto=0, Portrait=1, Landscape=2 };
inline bool Valid(int w,int h){return w>=2&&h>=2&&w<=2048&&h<=2048&&static_cast<long long>(w)*h<=1048576;}
inline bool Allowed(int w,int h,int orientation){return orientation==Auto||(orientation==Portrait&&h>=w)||(orientation==Landscape&&w>=h);}
inline bool CurrentGeometry(unsigned observedEpoch,unsigned targetEpoch,int observedW,int observedH,int targetW,int targetH){return observedEpoch==targetEpoch&&observedW==targetW&&observedH==targetH;}
struct Mapping {int sx,sy,sw,sh,dx,dy,dw,dh;};
inline Mapping Map(int sw,int sh,int dw,int dh,int fit){
    Mapping r={0,0,sw,sh,0,0,dw,dh};
    if(!Valid(sw,sh)||!Valid(dw,dh))return r;
    const long long source=static_cast<long long>(sw)*dh,target=static_cast<long long>(dw)*sh;
    if(fit==Cover){if(source>target){r.sw=static_cast<int>(static_cast<long long>(sh)*dw/dh);r.sx=(sw-r.sw)/2;}else{r.sh=static_cast<int>(static_cast<long long>(sw)*dh/dw);r.sy=(sh-r.sh)/2;}}
    else if(fit==Contain){if(source>target){r.dh=static_cast<int>(static_cast<long long>(dw)*sh/sw);r.dy=(dh-r.dh)/2;}else{r.dw=static_cast<int>(static_cast<long long>(dh)*sw/sh);r.dx=(dw-r.dw)/2;}}
    if(r.sw<1)r.sw=1;if(r.sh<1)r.sh=1;if(r.dw<1)r.dw=1;if(r.dh<1)r.dh=1;return r;
}
// Require two matching observations before rebuilding; a transient layout never
// grants permission to paint an old buffer into a new desktop geometry.
class SettledSize {
public: SettledSize():w(0),h(0),count(0){}
    void Reset(){w=h=count=0;}
    bool Observe(int width,int height){if(!Valid(width,height)){count=0;return false;}if(w!=width||h!=height){w=width;h=height;count=1;return false;}if(count<2)++count;return count>=2;}
private:int w,h,count;
};
}
#endif
