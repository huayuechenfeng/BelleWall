#ifndef BELLEWALL_DIAGNOSTICS_H
#define BELLEWALL_DIAGNOSTICS_H
#include <centralrepository.h>
#include <pslninternalcrkeys.h>
#include "log.h"
inline void diagnoseRepositoryL(TUid uid,TUint32 key,const char* label){
    CRepository* repository=CRepository::NewLC(uid);TInt value=-1;
    TInt error=repository->Get(key,value);
    logLine(QString("DIAG %1 value=%2 error=%3 (read only)").arg(label).arg(value).arg(error));
    CleanupStack::PopAndDestroy(repository);
}
inline void diagnoseL(){
    logLine("DIAG begin; repository reads only, no settings changed");
    TRAPD(themeError,diagnoseRepositoryL(KCRUidThemes,KThemesAnimBackgroundSupport,"theme-animation"));
    if(themeError)logLine(QString("DIAG theme repository open=%1").arg(themeError));
    TRAPD(variationError,diagnoseRepositoryL(TUid::Uid(0x102818eb),1,"theme-variation"));
    if(variationError)logLine(QString("DIAG variation repository open=%1").arg(variationError));
    logLine("DIAG complete");
}
#endif
