#include <cstdlib>
#include <cstring>
#include "../src/rendereridentity.h"
int main(int argc,char** argv){
    if(argc!=2)return 100;
    switch(std::atoi(argv[1])){
    case 1:return !(BelleRenderer::KnownVersion(1)&&BelleRenderer::ConfigUid(1)==0x70031119u&&!std::strcmp(BelleRenderer::ConfigText(1),"0x70031119"));
    case 2:return !(BelleRenderer::KnownVersion(2)&&BelleRenderer::ConfigUid(2)==0x70031129u&&!std::strcmp(BelleRenderer::ConfigText(2),"0x70031129"));
    case 3:return !(BelleRenderer::AcceptLongrun(2)&&!BelleRenderer::AcceptLongrun(1)&&BelleRenderer::ConfigUid(1)!=BelleRenderer::ConfigUid(2));
    case 4:{const unsigned unknown[]={0,3,0xffffffffu};for(unsigned i=0;i<3;i++)if(BelleRenderer::KnownVersion(unknown[i])||BelleRenderer::AcceptLongrun(unknown[i])||BelleRenderer::ConfigUid(unknown[i])||std::strlen(BelleRenderer::ConfigText(unknown[i])))return 1;return 0;}
    }return 101;
}
