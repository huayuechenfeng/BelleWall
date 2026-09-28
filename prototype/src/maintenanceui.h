#ifndef BELLEWALL_MAINTENANCE_UI_H
#define BELLEWALL_MAINTENANCE_UI_H
static void ExportWallpaperDiagnosticsL(const QString& destination){
    const QString base="C:/data/BelleWall/";
    // Fixed project-only allowlist, bounded tails; no library/media/ROM reads.
    QStringList names=QStringList()<<"host.log"<<"native-wallpaper.log"<<"render-plugin.log"<<"render-longrun.log"<<"candidate-metrics.csv"<<"candidate-last-result.txt"<<"preparation-pending.bin"<<"candidate-session.bin"<<"wallpaper-rollback.bin"<<"pages-rollback.bin";
    if(SywpNativePath(QDir::cleanPath(destination)).startsWith(SywpNativePath(base),Qt::CaseInsensitive))User::Leave(KErrArgument);
    QByteArray report="BelleWall 1.1 diagnostics\n";report+="Qt="+QByteArray(qVersion())+"\n";
    report+="Snapshot: files are read sequentially; active logs may change or be busy.\n";
    for(int i=0;i<names.size();i++){
        report+="\n--- "+names[i].toUtf8()+" ---\n";QFile file(base+names[i]);
        if(!file.exists()){report+="absent\n";continue;}
        if(!file.open(QIODevice::ReadOnly)){report+="unreadable or busy\n";continue;}
        const qint64 size=file.size();const bool binary=names[i].endsWith(".bin");const qint64 limit=binary?16384:65536;
        report+="bytes="+QByteArray::number(size)+"\n";
        if(size>limit){if(binary){report+="oversized record; contents omitted\n";continue;}report+="tail only\n";if(!file.seek(size-limit))User::Leave(KErrGeneral);}
        QByteArray data=file.read(limit);if(file.error()!=QFile::NoError)User::Leave(KErrGeneral);
        report+=binary?data.toHex():data;report+="\n";
    }
    SywpAtomicL(destination,report);
}
static void WallpaperMaintenance(int action){
    if(action==7){
        RFs fs;TVolumeInfo volume;TInt error=fs.Connect();if(!error){error=fs.Volume(volume,EDriveC);fs.Close();}
        QString summary=BwText("Qt：%1\nC 盘可用：%2\n").arg(QString::fromLatin1(qVersion())).arg(error?BwText("读取失败"):QString::number(double(volume.iFree)/1048576.0,'f',1)+" MiB");
        const char* records[]={"preparation-pending.bin","candidate-session.bin","pages-rollback.bin","wallpaper-rollback.bin"};int pending=0;for(int i=0;i<4;i++)if(QFile::exists(QString("C:/data/BelleWall/")+records[i]))++pending;
        summary+=BwText("会话／恢复记录：%1 项\n运行期间存在会话记录是正常的。\n可导出本应用日志供排查；不包含壁纸素材，不自动发送。").arg(pending);
        QDialog dialog;dialog.setWindowTitle(BwText("运行诊断"));QVBoxLayout* layout=new QVBoxLayout(&dialog);QScrollArea* scroll=new QScrollArea(&dialog);scroll->setWidgetResizable(true);QLabel* label=new QLabel(summary);label->setWordWrap(true);scroll->setWidget(label);layout->addWidget(scroll);
        QPushButton* exportButton=new QPushButton(BwText("导出诊断"),&dialog);QPushButton* close=new QPushButton(BwText("返回"),&dialog);layout->addWidget(exportButton);layout->addWidget(close);QObject::connect(exportButton,SIGNAL(clicked()),&dialog,SLOT(accept()));QObject::connect(close,SIGNAL(clicked()),&dialog,SLOT(reject()));
        dialog.showMaximized();if(dialog.exec()!=QDialog::Accepted)return;
        QString path=QFileDialog::getSaveFileName(0,BwText("保存诊断文件"),"E:/BelleWall-diagnostics.txt",BwText("诊断文本 (*.txt)"));if(path.isEmpty())return;
        TRAPD(saved,ExportWallpaperDiagnosticsL(path));QMessageBox::information(0,"BelleWall",saved?BwText("诊断导出失败，错误码：")+QString::number(saved):BwText("诊断已保存：\n")+path);return;
    }
    const bool cleanup=action==9;
    if(cleanup&&QMessageBox::question(0,"BelleWall",BwText("请先停止壁纸并完成桌面恢复。继续将移除 BelleWall 的组件注册，保留已导入壁纸；不会直接卸载应用。是否继续？"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
    TInt error=PrepareWallpaper(cleanup);
    QMessageBox::information(0,"BelleWall",error?PreparationError(error):BwText(cleanup?"组件注册已清理。请退出 BelleWall，再到系统应用管理依次卸载 BelleWall Preview、BelleWall Native Preview、BelleWall Renderer Helper。壁纸库保留；再次启动壁纸会重新准备组件。":"组件准备完成，尚未播放。请从壁纸管理选择并启动壁纸。"));
}
#endif
