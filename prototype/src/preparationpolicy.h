#ifndef BELLEWALL_PREPARATION_POLICY_H
#define BELLEWALL_PREPARATION_POLICY_H
// Positive rendezvous proves that an older helper did not fall through to its
// diagnostic command. Success additionally requires normal, zero-code exit.
namespace BellePreparation {
enum { Protocol=0x42575032, CleanupProtocol=0x42575033, Environment=-7101, Background=-7102, Components=-7103, JournalDamaged=-7104 };
inline bool Successful(int handshake,bool normalExit,int reason){return handshake==Protocol&&normalExit&&reason==0;}
struct Journal { unsigned magic,configuration,operation,check; };
inline unsigned Seal(const Journal& r){return r.magic^r.configuration^r.operation^0x9e3779b9u;}
inline bool SuccessfulFor(int handshake,int expected,bool normalExit,int reason){return (expected==Protocol||expected==CleanupProtocol)&&handshake==expected&&normalExit&&reason==0;}
inline bool Valid(const Journal& r){return r.magic==unsigned(Protocol)&&r.configuration==0x70031129u&&(r.operation>=1&&r.operation<=3)&&r.check==Seal(r);}
}
#endif
