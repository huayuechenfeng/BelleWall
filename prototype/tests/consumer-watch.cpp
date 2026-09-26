#include "../src/consumerwatch.h"
#include <assert.h>
#include <stdlib.h>
int main(int argc,char** argv){
    assert(argc==2);const int scenario=atoi(argv[1]);ConsumerWatch w(7,0);
    assert(w.Sample(7,0,true)==ConsumerWatch::Ready);
    if(scenario==1){for(int i=1;i<=20;i++)assert(w.Sample(7+i,i*100000LL,true)==ConsumerWatch::Ready);}
    else if(scenario==2){assert(w.Sample(7,2999999,true)==ConsumerWatch::Ready);assert(w.Sample(7,3000000,true)==ConsumerWatch::Waiting);assert(w.Sample(8,3500000,true)==ConsumerWatch::Ready);}
    else if(scenario==3){for(int i=1;i<10;i++)assert(w.Sample(7,i*1000000LL,true)==(i<3?ConsumerWatch::Ready:ConsumerWatch::Waiting));assert(w.Sample(7,10000000,true)==ConsumerWatch::Failed);}
    else if(scenario==4){assert(w.Sample(7,1000000,false)==ConsumerWatch::Ready);assert(w.Sample(7,86400000000LL,false)==ConsumerWatch::Ready);assert(w.Sample(7,86400100000LL,true)==ConsumerWatch::Ready);assert(w.Sample(7,86403100000LL,true)==ConsumerWatch::Waiting);}
    else if(scenario==5){assert(w.Sample(7,2000000,true)==ConsumerWatch::Ready);assert(w.Sample(7,20000000,true)==ConsumerWatch::Ready);assert(w.Sample(8,20100000,true)==ConsumerWatch::Ready);}
    else if(scenario==6){ConsumerWatch wrap(0xffffffffu,0);assert(wrap.Sample(0xffffffffu,0,true)==ConsumerWatch::Ready);assert(wrap.Sample(0,3000000,true)==ConsumerWatch::Ready);}
    else return 2;return 0;
}
