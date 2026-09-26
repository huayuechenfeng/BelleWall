#include "../src/librarydelete.h"
#include <cassert>
#include <cstdlib>
struct Backend {
    bool allowed,selected,payload,metadata,clearFail,metaFail,payloadFail;int mutations;
    Backend():allowed(true),selected(true),payload(true),metadata(true),clearFail(false),metaFail(false),payloadFail(false),mutations(0){}
    void Check(){if(!allowed)throw 1;}
    bool IsSelected(){return selected;}
    void ClearSelection(){if(clearFail)throw 2;selected=false;++mutations;}
    void RemoveMetadata(){if(metaFail)throw 3;metadata=false;++mutations;}
    void RemovePayload(){assert(!selected);if(payloadFail)throw 4;payload=false;++mutations;}
};
int main(int argc,char** argv){assert(argc==2);int n=std::atoi(argv[1]);Backend b;
    if(n==1)b.allowed=false;if(n==2)b.clearFail=true;if(n==3)b.metaFail=true;if(n==4)b.payloadFail=true;if(n==6)b.selected=false;
    int error=0;try{BelleLibrary::Delete(b);}catch(int e){error=e;}
    if(n==1||n==2){assert(error==n&&b.mutations==0&&b.selected&&b.payload&&b.metadata);}
    if(n==3){assert(error==3&&!b.selected&&b.payload&&b.metadata);b.metaFail=false;BelleLibrary::Delete(b);assert(!b.payload&&!b.metadata);}
    if(n==4){assert(error==4&&!b.selected&&b.payload&&!b.metadata);b.payloadFail=false;BelleLibrary::Delete(b);assert(!b.payload&&!b.metadata);}
    if(n==5||n==6)assert(!error&&!b.selected&&!b.payload&&!b.metadata&&b.mutations==(n==5?3:2));
    return 0;
}
