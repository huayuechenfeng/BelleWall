#ifndef BELLEWALL_PREPARATION_UI_H
#define BELLEWALL_PREPARATION_UI_H
#include <QtCore/QElapsedTimer>
#include <QtCore/QStringList>
#include "preparationpolicy.h"
// Qt pumps the Symbian active scheduler. Every outstanding request must have
// an active-object owner; polling bare TRequestStatus causes stray completions.
class CPreparationWait:public CActive {
public:
    CPreparationWait(RProcess& process,TBool handshake):CActive(EPriorityStandard),worker(process),rendezvous(handshake),finished(EFalse),result(KRequestPending){CActiveScheduler::Add(this);}
    ~CPreparationWait(){Cancel();}
    void Start(){if(rendezvous)worker.Rendezvous(iStatus);else worker.Logon(iStatus);SetActive();}
    TBool Done()const{return finished;}
    TInt Result()const{return result;}
private:
    void RunL(){result=iStatus.Int();finished=ETrue;}
    void DoCancel(){if(rendezvous)worker.RendezvousCancel(iStatus);else worker.LogonCancel(iStatus);}
    RProcess& worker;TBool rendezvous,finished;TInt result;
};
static TBool QtRuntimeMeetsBuildVersion(){
    const QStringList parts=QString::fromLatin1(qVersion()).split('.');
    if(parts.size()!=3)return EFalse;
    uint number[3];
    for(int i=0;i<3;i++){
        bool ok=false;number[i]=parts[i].toUInt(&ok);
        if(!ok||number[i]>255)return EFalse;
    }
    return QT_VERSION_CHECK(number[0],number[1],number[2])>=QT_VERSION;
}
static TInt PrepareWallpaper(TBool cleanup=EFalse){
    if(!cleanup&&!QtRuntimeMeetsBuildVersion())return BellePreparation::Environment;
    RProcess worker;TInt error=worker.Create(_L("C:\\sys\\bin\\bellerenderhost.exe"),cleanup?_L("--cleanup-wallpaper"):_L("--prepare-wallpaper"));if(error)return error;
    if(worker.SecureId().iId!=TInt(0xe7b31108)){worker.Kill(KErrPermissionDenied);worker.Close();return KErrPermissionDenied;}
    CPreparationWait ready(worker,ETrue),ended(worker,EFalse);ready.Start();ended.Start();
    const TInt expected=cleanup?BellePreparation::CleanupProtocol:BellePreparation::Protocol;
    QProgressDialog progress(BwText(cleanup?"正在核验并移除壁纸组件注册…":"正在检查手机环境并准备壁纸组件…"),QString(),0,0);
    progress.setCancelButton(0);progress.setWindowModality(Qt::ApplicationModal);progress.setMinimumDuration(0);progress.show();
    QElapsedTimer elapsed;elapsed.start();worker.Resume();
    while((!ended.Done()||!ready.Done())&&elapsed.elapsed()<120000){QApplication::processEvents();User::After(20000);}
    if(!ended.Done()||!ready.Done()){
        // A timeout never kills a registration transaction. The helper keeps
        // its mutex until it finishes; the next attempt queries actual identity.
        error=KErrTimedOut;
    }else if(BellePreparation::SuccessfulFor(ready.Result(),expected,worker.ExitType()==EExitKill,worker.ExitReason()))error=KErrNone;
    else if(worker.ExitType()==EExitKill&&worker.ExitReason()<0)error=worker.ExitReason();
    else if(ready.Result()!=expected)error=BellePreparation::Components;
    else error=worker.ExitType()==EExitKill&&worker.ExitReason()<0?worker.ExitReason():KErrGeneral;
    ready.Cancel();ended.Cancel();
    logLine(QString("UI preparation handshake=%1 result=%2").arg(ready.Result()).arg(error));worker.Close();progress.close();return error;
}
static QString PreparationError(TInt error){
    QString text;
    if(error==BellePreparation::JournalDamaged)text=BwText("组件准备记录损坏或不属于此版本，已保留记录。请通过「运行诊断」导出日志后反馈；不要卸载或删除记录。");
    else if(error==BellePreparation::Environment)text=BwText("当前桌面库布局未通过核验，或 Qt 运行库低于编译版本。请导出诊断；具体兼容范围见使用说明。");
    else if(error==BellePreparation::Background)text=BwText("请先将所有原生桌面页设为默认黑色背景，再重试播放。当前背景不会被替换。");
    else if(error==KErrInUse||error==KErrAlreadyExists)text=BwText("另一个操作尚未结束，或桌面仍待恢复。请先停止壁纸／重试恢复桌面，再播放。");
    else if(error==KErrDiskFull)text=BwText("手机 C 盘至少需要 8 MiB 可用空间来准备和恢复壁纸。请释放空间后重试。");
    else if(error==KErrTimedOut)text=BwText("组件准备耗时较长。请稍后重试；后台操作完成前不能播放。若持续如此，请保存日志反馈。");
    else text=BwText("壁纸组件准备失败。请确认三个配套安装包均已安装；若仍失败，请保存日志反馈，不要删除恢复记录。");
    return text+BwText("\n错误码：")+QString::number(error);
}
#endif
