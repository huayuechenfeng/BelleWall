#include <cstdlib>
#include "../src/preparationtransaction.h"
#include "../src/preparationpolicy.h"
struct Backend {
    int current,legacy,installs,removes,begins,commits,fault;bool journal;
    Backend(int c,int l):current(c),legacy(l),installs(0),removes(0),begins(0),commits(0),fault(0),journal(false){}
    int Count(unsigned v){if(v==1&&fault==1)throw 1;return v==2?current:legacy;}
    void Begin(unsigned){journal=true;begins++;if(fault==2)throw 2;}
    void Install(){installs++;if(fault!=5)current=1;if(fault==3)throw 3;}
    void Remove(unsigned v){removes++;if(v==2)current=0;else legacy=0;if(fault==4)throw 4;}
    void Fail(){throw 9;}
    void Commit(){commits++;journal=false;}
};
static bool fails(Backend& b,unsigned op){try{BellePreparation::Transaction(b,op);return false;}catch(int){return true;}}
int main(int argc,char** argv){
    if(argc!=2)return 100;
    switch(std::atoi(argv[1])){
    case 1:{Backend b(0,1);b.fault=2;if(!fails(b,1)||!b.journal||b.installs)return 1;b.fault=0;BellePreparation::Transaction(b,1);return b.journal||b.installs!=1||b.current!=1;}
    case 2:{Backend b(0,1);b.fault=3;if(!fails(b,1)||!b.journal||b.current!=1)return 1;b.fault=0;BellePreparation::Transaction(b,1);return b.journal||b.installs!=1;}
    case 3:{Backend b(1,1);b.fault=4;if(!fails(b,3)||!b.journal||b.current||b.legacy!=1)return 1;b.fault=0;BellePreparation::Transaction(b,3);return b.journal||b.current||b.legacy||b.removes!=2;}
    case 4:{Backend b(1,1);b.fault=1;return !fails(b,3)||b.begins||b.removes||b.current!=1;}
    case 5:{Backend b(2,1);return !fails(b,1)||b.begins||b.installs;}
    case 6:{Backend b(0,1);b.fault=5;return !fails(b,1)||!b.journal||b.commits;}
    case 7:{Backend b(0,0);BellePreparation::Transaction(b,3);BellePreparation::Transaction(b,3);return b.removes||b.journal||b.commits!=2;}
    case 8:{using namespace BellePreparation;Journal j={Protocol,0x70031129u,3,0};j.check=Seal(j);return !Valid(j)||SuccessfulFor(Protocol,CleanupProtocol,true,0)||SuccessfulFor(CleanupProtocol,Protocol,true,0)||!SuccessfulFor(CleanupProtocol,CleanupProtocol,true,0);}
    }return 101;
}
