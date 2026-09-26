#ifndef BELLEWALL_CANDIDATE_SESSION_H
#define BELLEWALL_CANDIDATE_SESSION_H
#include <e32base.h>
#include <f32file.h>
// One live coordinator owns this chunk. A durable record outlives it.
_LIT(KCandidateChunk,"BelleWallCandidateSessionV1");
_LIT(KCandidateJournal,"C:\\data\\BelleWall\\candidate-session.bin");
enum TCandidateState { EPreparing=1, EBinding, ERunning, EPaused, EStopping, ERecovering };
struct TCandidateRecord {
    TUint magic,version,nonceLo,nonceHi,owner,state,seconds,widgetIntent,widgetId;
    TUint crc;
};
struct TCandidateShared { TCandidateRecord record; volatile TInt stop; volatile TUint heartbeat,consumer; volatile TInt verified; };
static TUint CandidateCrc(const TCandidateRecord& r){const TUint8* p=reinterpret_cast<const TUint8*>(&r);TUint c=2166136261u;for(TUint i=0;i<sizeof(r)-4;i++)c=(c^p[i])*16777619u;return c;}
static void CandidateReadL(RFs& fs,TCandidateRecord& r){RFile f;User::LeaveIfError(f.Open(fs,KCandidateJournal,EFileRead|EFileShareReadersOnly));CleanupClosePushL(f);TInt n;User::LeaveIfError(f.Size(n));if(n!=sizeof(r))User::Leave(KErrCorrupt);TPtr8 b(reinterpret_cast<TUint8*>(&r),sizeof(r),sizeof(r));User::LeaveIfError(f.Read(b));if(b.Length()!=sizeof(r)||r.magic!=0x31535742||r.version!=1||r.crc!=CandidateCrc(r)||r.state<EPreparing||r.state>ERecovering||r.seconds<1||r.seconds>600)User::Leave(KErrCorrupt);CleanupStack::PopAndDestroy(&f);}
static void CandidateWriteL(RFs& fs,TCandidateRecord& r){r.crc=CandidateCrc(r);TFileName tmp(KCandidateJournal);tmp.Append(_L(".tmp"));RFile f;User::LeaveIfError(f.Replace(fs,tmp,EFileWrite|EFileShareExclusive));CleanupClosePushL(f);User::LeaveIfError(f.Write(TPtrC8(reinterpret_cast<const TUint8*>(&r),sizeof(r))));User::LeaveIfError(f.Flush());CleanupStack::PopAndDestroy(&f);User::LeaveIfError(fs.Replace(tmp,KCandidateJournal));}
static TBool CandidateSame(const TCandidateRecord& a,const TCandidateRecord& b){return a.nonceLo==b.nonceLo&&a.nonceHi==b.nonceHi&&a.owner==b.owner;}
static void CandidateToken(const TCandidateRecord& r,TDes& token){token.Format(_L("%08x%08x"),r.nonceHi,r.nonceLo);}
static TBool CandidateArgsMatch(const TCandidateRecord& r,const TDesC& args){TBuf<16> token;CandidateToken(r,token);return args.Right(16)==token;}
static TUint CandidateFileHashL(RFile& f,TInt size){TUint hash=2166136261u;TBuf8<512> b;for(TInt pos=0;pos<size;){User::LeaveIfError(f.Read(pos,b,Min(512,size-pos)));if(!b.Length())User::Leave(KErrCorrupt);for(TInt i=0;i<b.Length();i++)hash=(hash^b[i])*16777619u;pos+=b.Length();}return hash;}
static void CandidateSealL(RFs& fs,const TDesC& path){RFile f;User::LeaveIfError(f.Open(fs,path,EFileWrite|EFileRead|EFileShareExclusive));CleanupClosePushL(f);TInt size;User::LeaveIfError(f.Size(size));TUint hash=CandidateFileHashL(f,size);User::LeaveIfError(f.Write(size,TPckgC<TUint>(hash)));User::LeaveIfError(f.Flush());CleanupStack::PopAndDestroy(&f);}
static void CandidateVerifyL(RFs& fs,const TDesC& path){RFile f;User::LeaveIfError(f.Open(fs,path,EFileRead|EFileShareReadersOnly));CleanupClosePushL(f);TInt size;User::LeaveIfError(f.Size(size));if(size<4||size>16384)User::Leave(KErrCorrupt);TPckgBuf<TUint> hash;User::LeaveIfError(f.Read(size-4,hash));if(hash.Length()!=4||hash()!=CandidateFileHashL(f,size-4))User::Leave(KErrCorrupt);CleanupStack::PopAndDestroy(&f);}
// RChunk rounds committed memory up to a system page; Size is not the struct size.
static TCandidateShared* CandidateOpen(RChunk& chunk){if(chunk.OpenGlobal(KCandidateChunk,EFalse))return 0;if(chunk.Size()<TInt(sizeof(TCandidateShared))||chunk.Size()>65536){chunk.Close();return 0;}TCandidateShared* s=reinterpret_cast<TCandidateShared*>(chunk.Base());if(s->record.magic!=0x31535742||s->record.version!=1){chunk.Close();return 0;}return s;}
#endif
