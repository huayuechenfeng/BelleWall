#ifndef BELLEWALL_CONSUMER_WATCH_H
#define BELLEWALL_CONSUMER_WATCH_H
// Pure timing policy, shared by ARM runtime and host tests. A stalled consumer
// first pauses content. Fresh progress or a new observation window resets age.
class ConsumerWatch {
public:
    enum Health { Ready, Waiting, Failed };
    ConsumerWatch(unsigned int counter,long long now):counter_(counter),seen_(now),sample_(now),foreground_(false){}
    Health Sample(unsigned int counter,long long now,bool foreground){
        const bool observationGap=now-sample_>5000000;
        if(counter!=counter_||!foreground||!foreground_||observationGap)seen_=now;
        counter_=counter;sample_=now;foreground_=foreground;
        if(!foreground)return Ready;
        const long long age=now-seen_;
        return age>=10000000?Failed:age>=3000000?Waiting:Ready;
    }
    long long Age(long long now)const{return now-seen_;}
private:
    unsigned int counter_;long long seen_,sample_;bool foreground_;
};
#endif
