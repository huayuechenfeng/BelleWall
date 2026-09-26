#include <cstdlib>
#include "../src/preparationpolicy.h"
int main(int argc,char** argv){
    if(argc!=2)return 100;
    using namespace BellePreparation;
    switch(std::atoi(argv[1])){
    case 1:return !Successful(Protocol,true,0);
    case 2:return Successful(0,true,0)||Successful(1,true,0)||Successful(-1,true,0);
    case 3:return Successful(Protocol,false,0)||Successful(Protocol,false,-1);
    case 4:{const int errors[]={-1,-4,-14,-20,-33,Environment,Background,Components,1};for(unsigned i=0;i<sizeof(errors)/sizeof(errors[0]);i++)if(Successful(Protocol,true,errors[i]))return 1;return 0;}
    case 5:{Journal r={Protocol,0x70031129u,1,0};r.check=Seal(r);if(sizeof(r)!=16||!Valid(r))return 1;r.operation=2;r.check=Seal(r);return !Valid(r);}
    case 6:{Journal good={Protocol,0x70031129u,1,0};good.check=Seal(good);for(unsigned i=0;i<4;i++){Journal bad=good;reinterpret_cast<unsigned*>(&bad)[i]^=1;if(Valid(bad))return 1;}Journal unknown={Protocol,0x70031119u,1,0};unknown.check=Seal(unknown);if(Valid(unknown))return 1;unknown.configuration=0x70031129u;unknown.operation=4;unknown.check=Seal(unknown);return Valid(unknown);}
    }return 101;
}
