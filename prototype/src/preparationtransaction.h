#ifndef BELLEWALL_PREPARATION_TRANSACTION_H
#define BELLEWALL_PREPARATION_TRANSACTION_H
namespace BellePreparation {
// Backend verifies identity while counting. Both identities are checked before
// full cleanup can mutate either registration. Commit alone clears the journal.
template<class Backend> void Transaction(Backend& b,unsigned operation){
    if(operation<1||operation>3){b.Fail();return;}
    int current=b.Count(2),legacy=operation==3?b.Count(1):0;
    if(current<0||current>1||legacy<0||legacy>1){b.Fail();return;}
    b.Begin(operation);
    if(operation==1){if(!current)b.Install();if(b.Count(2)!=1){b.Fail();return;}}
    else {
        if(current)b.Remove(2);if(b.Count(2)!=0){b.Fail();return;}
        if(operation==3){if(legacy)b.Remove(1);if(b.Count(1)!=0){b.Fail();return;}}
    }
    b.Commit();
}
}
#endif
