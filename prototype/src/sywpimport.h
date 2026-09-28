#ifndef BELLEWALL_SYWP_IMPORT_H
#define BELLEWALL_SYWP_IMPORT_H
#include <hash.h>
#include "librarydelete.h"
#include <QtScript/QScriptEngine>
#include <QtGui/QInputDialog>
#include <QtGui/QListWidget>
#include <QtCore/QFileInfo>
#include <QtGui/QHBoxLayout>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QDialog>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QProgressDialog>
#include <QtGui/QScrollArea>
#include "uilanguage.h"
#include <QtCore/QSignalMapper>
#include <QtCore/QElapsedTimer>
#include <apgtask.h>
#include "candidatesession.h"
#include "sywpstorage.h"
#include "displaypolicy.h"
static quint32 SywpU32(const QByteArray& b,int at){const unsigned char* p=reinterpret_cast<const unsigned char*>(b.constData()+at);return quint32(p[0])|(quint32(p[1])<<8)|(quint32(p[2])<<16)|(quint32(p[3])<<24);}
static quint32 SywpBe32(const QByteArray& b,int at){const unsigned char* p=reinterpret_cast<const unsigned char*>(b.constData()+at);return (quint32(p[0])<<24)|(quint32(p[1])<<16)|(quint32(p[2])<<8)|quint32(p[3]);}
static void SywpPut(QByteArray& b,int at,quint32 n){for(int i=0;i<4;i++)b[at+i]=char(n>>(8*i));}
static TPtrC SywpDes(const QString& s){return TPtrC(reinterpret_cast<const TUint16*>(s.utf16()),s.length());}
static QString SywpNativePath(QString s){return s.replace(QChar('/'),QChar('\\'));}
static quint32 SywpIntegerL(const QScriptValue& m,const char* name,quint32 maximum){QScriptValue v=m.property(name);double n=v.toNumber();if(!v.isNumber()||!(n>=0&&n<=maximum)||n!=quint32(n))User::Leave(KErrCorrupt);return quint32(n);}
static void SywpLockLC(RMutex& lock){User::LeaveIfError(lock.CreateGlobal(_L("BelleWallWallpaperExperiment")));CleanupClosePushL(lock);if(QFile::exists("C:/data/BelleWall/candidate-session.bin")||QFile::exists("C:/data/BelleWall/pages-rollback.bin")||QFile::exists("C:/data/BelleWall/wallpaper-rollback.bin"))User::Leave(KErrInUse);}
static void SywpAtomicL(const QString& target,const QByteArray& data){
    const QString temp=target+".tmp";QFile f(temp);if(!f.open(QIODevice::WriteOnly|QIODevice::Truncate)||f.write(data)!=data.size()||!f.flush()){f.close();QFile::remove(temp);User::Leave(KErrWrite);}f.close();
    RFs fs;User::LeaveIfError(fs.Connect());TInt error=fs.Replace(SywpDes(SywpNativePath(temp)),SywpDes(SywpNativePath(target)));fs.Close();if(error)QFile::remove(temp);User::LeaveIfError(error);
}
static void SywpSelectUnlockedL(const QString& name,bool web){
    const QString id=SywpStorageId(name);
    if(id.isEmpty()||!(web?id.endsWith(".html"):(id.endsWith(".bwv")||id.endsWith(".mp4"))))User::Leave(KErrCorrupt);
    if(!QFile::exists(SywpStoragePath(id)))User::Leave(KErrNotFound);
    QFile metadata(SywpStoragePath(id)+".json");if(!metadata.open(QIODevice::ReadOnly)||metadata.size()>16384)User::Leave(KErrCorrupt);
    QScriptEngine parser;QString escaped="\"",text=QString::fromUtf8(metadata.readAll());for(int i=0;i<text.size();i++)escaped+=QString("\\u%1").arg(text.at(i).unicode(),4,16,QChar('0'));escaped+="\"";
    QScriptValue manifest=parser.evaluate("JSON.parse("+escaped+")"),display=manifest.property("display");if(parser.hasUncaughtException())User::Leave(KErrCorrupt);
    QByteArray policy(28,0);SywpPut(policy,0,0x31445742);SywpPut(policy,4,manifest.property("width").toUInt32());SywpPut(policy,8,manifest.property("height").toUInt32());
    QString fit=display.property("fit").toString(),orientation=display.property("orientation").toString();SywpPut(policy,12,fit=="contain"?1:fit=="stretch"?2:0);SywpPut(policy,16,orientation=="portrait"?1:orientation=="landscape"?2:0);SywpPut(policy,20,display.property("background").toString().mid(1).toUInt(0,16));SywpAtomicL(SywpStoragePath(id)+".display",policy);
    SywpAtomicL("C:/data/BelleWall/selected-wallpaper.txt",id.toLatin1());
}
static void SywpSelectL(const QString& name,bool web){RMutex lock;SywpLockLC(lock);SywpSelectUnlockedL(name,web);CleanupStack::PopAndDestroy(&lock);}
class SywpTemporary {public:explicit SywpTemporary(const QString& p):path(p){}~SywpTemporary(){QFile::remove(path);}QString path;};
static QChar SywpImportDrive(){QFile f("C:/data/BelleWall/library-drive.txt");if(f.open(QIODevice::ReadOnly)&&f.size()==1){QByteArray value=f.readAll();if(value=="C"||value=="E"||value=="F")return QChar(value[0]);}return QChar('C');}
static void SywpImportL(const QString& input,QChar destination=QChar()){
    RMutex lock;SywpLockLC(lock);
    QElapsedTimer total;total.start();
    if(destination.isNull())destination=SywpImportDrive();
    if(destination!='C'&&destination!='E'&&destination!='F')User::Leave(KErrArgument);
    QFile file(input);if(!file.open(QIODevice::ReadOnly))User::Leave(KErrNotFound);QByteArray h=file.read(64);
    if(h.size()!=64||h.left(4)!="SYWP"||SywpU32(h,4)!=1||SywpU32(h,8)!=64||SywpU32(h,20)!=0||h.mid(56)!=QByteArray(8,0))User::Leave(KErrCorrupt);
    quint32 manifestSize=SywpU32(h,12),payload=SywpU32(h,16);if(!manifestSize||manifestSize>16384||payload>256u*1024*1024||file.size()!=qint64(64)+manifestSize+payload)User::Leave(KErrCorrupt);
    QByteArray json=file.read(manifestSize);QString decoded=QString::fromUtf8(json);if(decoded.toUtf8()!=json)User::Leave(KErrCorrupt);
    // JSON.parse receives an escaped string, never executable manifest text.
    QString quoted="\"";for(int i=0;i<decoded.size();i++)quoted+=QString("\\u%1").arg(decoded.at(i).unicode(),4,16,QChar('0'));quoted+="\"";
    QScriptEngine engine;QScriptValue m=engine.evaluate("JSON.parse("+quoted+")");if(engine.hasUncaughtException()||!m.isObject())User::Leave(KErrCorrupt);
    const bool web=m.property("kind").toString()=="web",mp4=!web&&m.property("container").toString()=="mp4";QString kind=m.property("kind").toString();QScriptValue display=m.property("display");
    if(m.property("format").toString()!="sywp"||SywpIntegerL(m,"version",1)!=1||(!web&&kind!="video")||!m.property("loop").isBool()||m.property("loop").toBool()!=true||m.property("pause").toString()!="resume")User::Leave(KErrNotSupported);
    if(!m.property("title").isString()||m.property("title").toString().trimmed().isEmpty()||m.property("title").toString().size()>120||!QRegExp("#[0-9a-fA-F]{6}").exactMatch(display.property("background").toString()))User::Leave(KErrCorrupt);
    QScriptValue features=m.property("requiredFeatures");
    if(mp4){if(!features.isArray()||features.property("length").toInt32()!=1||features.property(0).toString()!="video-mp4v-v1")User::Leave(KErrNotSupported);}
    else if(features.isValid()&&(!features.isArray()||features.property("length").toInt32()!=0))User::Leave(KErrNotSupported);
    // Format is broader than this firmware-specific playback profile.
    int w=SywpIntegerL(m,"width",2048),height=SywpIntegerL(m,"height",2048);
    if(!BelleDisplay::Valid(w,height)||(w&1))User::Leave(KErrNotSupported);
    if(display.property("rotation").toString()!="follow-display"||!(display.property("orientation").toString()=="auto"||display.property("orientation").toString()=="portrait"||display.property("orientation").toString()=="landscape"))User::Leave(KErrNotSupported);
    if(!(display.property("fit").toString()=="cover"||display.property("fit").toString()=="contain"||display.property("fit").toString()=="stretch"))User::Leave(KErrNotSupported);
    quint32 count=web?0:SywpIntegerL(m,"frames",mp4?100000:268435456),num=web?0:SywpIntegerL(m,"fpsNumerator",60000),den=web?0:SywpIntegerL(m,"fpsDenominator",1001);
    if(web){if(m.property("entry").toString()!="index.html"||payload>256*1024||!payload)User::Leave(KErrCorrupt);}
    else if(mp4){if(m.property("codec").toString()!="mpeg4-part2"||m.property("pixelFormat").isValid()||m.property("stride").isValid()||payload<32||!count||!num||!den||num<den||num>den*30)User::Leave(KErrCorrupt);}
    else if(m.property("container").isValid()||m.property("codec").isValid()||m.property("pixelFormat").toString()!="rgb565le"||SywpIntegerL(m,"stride",4096)!=quint32(w*2)||!count||quint64(count)*w*height*2!=payload||!num||num>60000||!den||den>1001||num<den||num>den*60)User::Leave(KErrCorrupt);
    const QString dir=QString(destination)+":/data/BelleWall/library/";
    RFs space;User::LeaveIfError(space.Connect());TVolumeInfo volume;TInt drive=destination=='C'?EDriveC:destination=='E'?EDriveE:EDriveF;TInt volumeError=space.Volume(volume,drive);space.Close();User::LeaveIfError(volumeError);if(volume.iFree<TInt64(payload)+manifestSize+65536)User::Leave(KErrDiskFull);
    if(!QDir().mkpath(dir))User::Leave(KErrWrite);
    QString name=QString::fromLatin1(h.mid(24,32).toHex())+(web?".html":mp4?".mp4":".bwv"),temp=dir+name+".tmp";SywpTemporary pending(temp);QFile out(temp);if(!out.open(QIODevice::WriteOnly|QIODevice::Truncate))User::Leave(KErrWrite);
    const quint32 chunk=1024u*1024;
    QProgressDialog progress(BwText("正在校验并导入壁纸…"),BwText("取消"),0,int((payload+chunk-1)/chunk));progress.setWindowModality(Qt::ApplicationModal);progress.setMinimumDuration(0);progress.setValue(0);
    if(!web&&!mp4){QByteArray bwv(48,0);quint32 fields[]={0x32565742,48,quint32(w),quint32(height),quint32(w*2),1,num,den,count,quint32(w*height*2),48,payload};for(int i=0;i<12;i++)SywpPut(bwv,i*4,fields[i]);if(out.write(bwv)!=48)User::Leave(KErrWrite);}
    CSHA2* hash=CSHA2::NewLC(E256Bit);hash->Update(TPtrC8(reinterpret_cast<const TUint8*>(json.constData()),json.size()));QByteArray html;quint32 remaining=payload;QElapsedTimer stage;stage.start();qint64 readMs=0,hashMs=0,writeMs=0,uiMs=0,flushMs=0,lastProgressMs=0;
    QByteArray b; b.resize(chunk);while(remaining){
        stage.restart();const qint64 got=file.read(b.data(),qMin(remaining,chunk));readMs+=stage.elapsed();if(got<=0)User::Leave(KErrCorrupt);
        if(mp4&&remaining==payload&&(got<16||SywpBe32(b,0)<16||SywpBe32(b,0)>payload||b.mid(4,4)!="ftyp"))User::Leave(KErrCorrupt);
        stage.restart();hash->Update(TPtrC8(reinterpret_cast<const TUint8*>(b.constData()),got));hashMs+=stage.elapsed();
        if(web)html.append(b.constData(),got);
        stage.restart();if(out.write(b.constData(),got)!=got)User::Leave(KErrWrite);writeMs+=stage.elapsed();remaining-=got;
        if(total.elapsed()-lastProgressMs>=250||!remaining){stage.restart();progress.setValue(int((payload-remaining+chunk-1)/chunk));uiMs+=stage.elapsed();lastProgressMs=total.elapsed();}
        if(progress.wasCanceled())User::Leave(KErrCancel);
    }
    TPtrC8 digest=hash->Final();QByteArray result(reinterpret_cast<const char*>(digest.Ptr()),digest.Length());CleanupStack::PopAndDestroy(hash);if(result!=h.mid(24,32)||(web&&(!html.contains("bellewallStep")||QString::fromUtf8(html).toUtf8()!=html))){out.close();QFile::remove(temp);User::Leave(KErrCorrupt);}stage.restart();if(!out.flush())User::Leave(KErrWrite);out.close();flushMs=stage.elapsed();
    RFs fs;User::LeaveIfError(fs.Connect());TInt rename=fs.Replace(SywpDes(SywpNativePath(temp)),SywpDes(SywpNativePath(dir+name)));fs.Close();User::LeaveIfError(rename);
    SywpAtomicL(dir+name+".json",json);if(!QDir().mkpath("C:/data/BelleWall"))User::Leave(KErrWrite);SywpAtomicL("C:/data/BelleWall/library-drive.txt",QString(destination).toLatin1());SywpSelectUnlockedL(QString(destination)+":"+name,web);progress.setValue(progress.maximum());
    logLine(QString("SYWP import profile bytes=%1 drive=%2 total_ms=%3 read_ms=%4 hash_ms=%5 write_ms=%6 ui_ms=%7 flush_ms=%8 chunk_bytes=%9").arg(payload).arg(destination).arg(total.elapsed()).arg(readMs).arg(hashMs).arg(writeMs).arg(uiMs).arg(flushMs).arg(chunk));
    CleanupStack::PopAndDestroy(&lock);
}
static void CandidateShowDesktopL(){
    RWsSession ws;User::LeaveIfError(ws.Connect());CleanupClosePushL(ws);TApaTaskList tasks(ws);TApaTask desktop=tasks.FindApp(TUid::Uid(0x102750f0));
    if(!desktop.Exists())User::Leave(KErrNotReady);desktop.BringToForeground();CleanupStack::PopAndDestroy(&ws);
}
static void CandidateLaunchL(const QString& command){
    // Only a direct user action brings the native desktop forward; no page is selected.
    CandidateShowDesktopL();RProcess p;TInt error=p.Create(_L("C:\\sys\\bin\\bellepaper.exe"),SywpDes(command));logLine(QString("UI command=%1 create_result=%2").arg(command).arg(error));User::LeaveIfError(error);p.Resume();p.Close();
}
static QString SywpChosen(){QFile f("C:/data/BelleWall/selected-wallpaper.txt");if(!f.open(QIODevice::ReadOnly)||f.size()>80)return QString();return SywpStorageId(QString::fromLatin1(f.readAll()));}
static QString SywpLabel(const QString& name){
    QString title=BwText(name.endsWith(".html")?"网页壁纸":"视频壁纸")+" "+name.mid(2,8);QFile f(SywpStoragePath(name)+".json");
    if(f.open(QIODevice::ReadOnly)&&f.size()<=16384){QString json=QString::fromUtf8(f.readAll()),quoted="\"";for(int i=0;i<json.size();i++)quoted+=QString("\\u%1").arg(json.at(i).unicode(),4,16,QChar('0'));quoted+="\"";QScriptEngine engine;QScriptValue m=engine.evaluate("JSON.parse("+quoted+")");if(!engine.hasUncaughtException()&&m.property("title").isString()){title=m.property("title").toString().left(120);title+=BwText(" · %1×%2").arg(m.property("width").toInt32()).arg(m.property("height").toInt32());}}
    return title;
}
// Keep the library visible in one Qt dialog; no native combo popup or hidden
// settings dialog/timer remains alive while the user chooses a wallpaper.
static QString SywpPickWallpaper(const QString& chosen,int& action){
    action=0;QStringList names,valid;
    for(const char* drive="CEF";*drive;drive++){
        QString root=QString(QChar(*drive))+":/data/BelleWall/library/";
        QStringList entries=QDir(root).entryList(QStringList()<<"*.bwv"<<"*.mp4"<<"*.html",QDir::Files);
        for(int i=0;i<entries.size();i++)names<<QString(QChar(*drive))+":"+entries[i];
    }
    QDialog picker;picker.setWindowTitle(BwText("壁纸管理"));QVBoxLayout* layout=new QVBoxLayout(&picker);
    QLabel* info=new QLabel(&picker);info->setWordWrap(true);layout->addWidget(info);
    QListWidget* list=new QListWidget(&picker);list->setWordWrap(true);layout->addWidget(list);int current=0;qint64 total=0;
    for(int i=0;i<names.size();i++)if(!SywpStorageId(names[i]).isEmpty()){
        if(names[i]==chosen)current=valid.size();valid<<names[i];
        qint64 bytes=QFileInfo(SywpStoragePath(names[i])).size()+QFileInfo(SywpStoragePath(names[i])+".json").size();total+=bytes;
        QString text=SywpLabel(names[i])+BwText("\n%1 盘 · %2 MiB%3").arg(names[i].left(1)).arg(double(bytes)/1048576.0,0,'f',2).arg(names[i]==chosen?BwText(" · 已选中"):QString());
        QListWidgetItem* item=new QListWidgetItem(text,list);item->setSizeHint(QSize(0,64));
    }
    RFs fs;QStringList freeSpace;if(fs.Connect()==KErrNone){for(const char* candidate="CEF";*candidate;candidate++){TVolumeInfo volume;const TInt drive=*candidate=='C'?EDriveC:*candidate=='E'?EDriveE:EDriveF;if(fs.Volume(volume,drive)==KErrNone)freeSpace<<BwText("%1 盘可用 %2 MiB").arg(QChar(*candidate)).arg(double(volume.iFree)/1048576.0,0,'f',1);}fs.Close();}
    info->setText(BwText("壁纸 %1 MiB · %2\n%3").arg(double(total)/1048576.0,0,'f',2).arg(freeSpace.isEmpty()?BwText("可用空间暂不可读"):freeSpace.join(BwText(" · "))).arg(valid.isEmpty()?BwText("还没有壁纸，请导入 SYWP 壁纸包。"):BwText("选一张壁纸，可预览、删除或启动。")));
    list->setCurrentRow(current);QSignalMapper* mapper=new QSignalMapper(&picker);
    const QStringList labels=QStringList()<<BwText("导入壁纸包")<<BwText("预览壁纸")<<BwText("删除壁纸")<<BwText("启动动态壁纸")<<BwText("返回");
    const int ids[]={3,4,2,1,0};
    QHBoxLayout* row=new QHBoxLayout;layout->addLayout(row);
    for(int i=0;i<5;i++){QPushButton* button=new QPushButton(labels[i],&picker);button->setMinimumHeight(42);if(i<3)row->addWidget(button);else layout->addWidget(button);
        if(i==1||i==2||i==3)button->setEnabled(!valid.isEmpty());
        if(i==3)button->setStyleSheet("QPushButton:enabled { background-color: #9de8c6; color: #142c24; font-weight: bold; }");
        QObject::connect(button,SIGNAL(clicked()),mapper,SLOT(map()));mapper->setMapping(button,ids[i]);
    }
    QObject::connect(mapper,SIGNAL(mapped(int)),&picker,SLOT(done(int)));
    logLine(QString("UI library ready count=%1").arg(valid.size()));picker.showMaximized();action=picker.exec();int rowIndex=list->currentRow();
    logLine(QString("UI library action=%1 row=%2").arg(action).arg(rowIndex));
    return rowIndex>=0&&rowIndex<valid.size()?valid[rowIndex]:QString();
}
class SywpDeleteBackend {
public:
    explicit SywpDeleteBackend(const QString& value):name(value){}
    void Check(){
        if(SywpStorageId(name).isEmpty())User::Leave(KErrArgument);
        if(QFile::exists("C:/data/BelleWall/preparation-pending.bin"))User::Leave(KErrInUse);
        if(!QFileInfo(SywpStoragePath(name)).isFile())User::Leave(KErrNotFound);
    }
    bool IsSelected(){return SywpChosen()==name;}
    void ClearSelection(){SywpAtomicL("C:/data/BelleWall/selected-wallpaper.txt",QByteArray());}
    void RemovePayload(){Remove(SywpStoragePath(name));}
    void RemoveMetadata(){Remove(SywpStoragePath(name)+".display");Remove(SywpStoragePath(name)+".json");}
private:
    void Remove(const QString& path){if(QFile::exists(path)&&!QFile::remove(path))User::Leave(KErrWrite);}
    QString name;
};
static void SywpDeleteL(const QString& name){RMutex lock;SywpLockLC(lock);SywpDeleteBackend backend(name);BelleLibrary::Delete(backend);CleanupStack::PopAndDestroy(&lock);}
static void SywpConfirmDelete(const QString& name){
    QString message=BwText("删除手机库中的「%1」？\n素材和元数据将被删除，手机上的原始 SYWP 文件保留。").arg(SywpLabel(name));
    if(name==SywpChosen())message+=BwText("\n这张壁纸已选中；删除时将取消选择。");
    if(QMessageBox::question(0,"BelleWall",message,QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
    TRAPD(error,SywpDeleteL(name));logLine(QString("UI library delete result=%1").arg(error));
    QMessageBox::information(0,"BelleWall",error?BwText("删除未完成（错误码 %1）。运行或恢复期间不能删除。若已取消选择或部分删除，请重新查看列表；原始 SYWP 文件未删除。").arg(error):BwText("壁纸已从手机库删除，原始 SYWP 文件保留。"));
}
static QString SywpError(TInt error){
    if(error==KErrCancel)return BwText("已取消导入，原来的壁纸选择没有改变。");
    if(error==KErrInUse||error==KErrAlreadyExists)return BwText("请先停止壁纸并完成桌面恢复，再导入或选择内容。");
    if(error==KErrDiskFull)return BwText("选定盘空间不足。请换一个存储盘或释放空间后重试，原来的壁纸选择没有改变。");
    if(error==KErrNotSupported)return BwText("这个壁纸包暂不适用于当前手机。画布最多 2048 边长、100 万像素；MP4 还依赖系统解码器。");
    if(error==KErrCorrupt)return BwText("壁纸包损坏或格式不正确，请重新生成或复制。原来的壁纸选择没有改变。");
    if(error==KErrNotFound)return BwText("找不到文件，请重新选择或导入。");
    return BwText("操作未完成，请检查文件和可用空间后重试。错误码：")+QString::number(error);
}
class SywpPreviewDialog:public QDialog {
public:
    SywpPreviewDialog():view(new QLabel(this)),note(new QLabel(this)),web(false),timer(0),width(0),height(0),count(0),num(0),den(1),activeMs(0),lastPreviewMs(0){
        setWindowTitle(BwText("壁纸预览"));QVBoxLayout* layout=new QVBoxLayout(this);note->setWordWrap(true);layout->addWidget(note);view->setAlignment(Qt::AlignCenter);layout->addWidget(view,1);
        QPushButton* close=new QPushButton(BwText("返回壁纸管理"),this);close->setMinimumHeight(44);layout->addWidget(close);QObject::connect(close,SIGNAL(clicked()),this,SLOT(reject()));
    }
    ~SywpPreviewDialog(){if(timer)killTimer(timer);}
    void OpenL(const QString& name){
        if(SywpStorageId(name).isEmpty())User::Leave(KErrArgument);
        web=name.endsWith(".html");note->setText(SywpLabel(name)+BwText(web?"\n应用内网页预览（5 fps），不会应用到桌面。":"\n应用内视频预览（最高 30 fps），不会应用到桌面。"));QString path=SywpStoragePath(name);
        if(name.endsWith(".mp4")){note->setText(SywpLabel(name)+BwText("\nMP4 压缩版的应用内预览尚未接入。可返回壁纸管理启动测试；若系统解码器不支持，请停止并保留诊断。"));return;}
        if(web){width=180;height=320;QFile display(path+".display");if(display.open(QIODevice::ReadOnly)){QByteArray data=display.read(28);if(data.size()==28){width=SywpU32(data,4);height=SywpU32(data,8);}}if(!BelleDisplay::Valid(width,height))User::Leave(KErrCorrupt);content.loadL(path);content.resumeL(QSize(width,height));}
        else{file.setFileName(path);if(!file.open(QIODevice::ReadOnly))User::Leave(KErrNotFound);QByteArray h=file.read(48);
            if(h.size()!=48||SywpU32(h,0)!=0x32565742||SywpU32(h,4)!=48)User::Leave(KErrCorrupt);
            width=SywpU32(h,8);height=SywpU32(h,12);num=SywpU32(h,24);den=SywpU32(h,28);count=SywpU32(h,32);
            if(!BelleDisplay::Valid(width,height)||(width&1)||SywpU32(h,16)!=width*2||SywpU32(h,20)!=1||!count||!num||!den||den>1001||num<den||num>den*60||SywpU32(h,36)!=width*height*2||SywpU32(h,40)!=48||qint64(SywpU32(h,44))!=qint64(count)*width*height*2||file.size()!=48+qint64(count)*width*height*2)User::Leave(KErrCorrupt);
        }
        previewClock.start();lastPreviewMs=previewClock.elapsed();timer=startTimer(web?200:qMax(33,int(1000LL*den/num)));logLine("UI preview start");
    }
protected:
    void timerEvent(QTimerEvent*){
        const qint64 now=previewClock.elapsed();if(!isActiveWindow()){lastPreviewMs=now;if(web)content.pause();return;}
        const qint64 delta=now-lastPreviewMs;if(delta>0&&delta<5000)activeMs+=delta;lastPreviewMs=now;
        TRAPD(error,FrameL());if(error){killTimer(timer);timer=0;note->setText(BwText("预览失败，错误码：%1。壁纸选择和桌面未改变。").arg(error));logLine(QString("UI preview error=%1").arg(error));}
    }
private:
    void FrameL(){QImage frame;
        if(web){content.resumeL(QSize(width,height));frame=content.frameL(200);}
        else{const qint64 index=(activeMs*num/(1000*den))%count;const int bytes=width*height*2;if(!file.seek(48+index*bytes))User::Leave(KErrCorrupt);QByteArray raw=file.read(bytes);if(raw.size()!=bytes)User::Leave(KErrCorrupt);
            frame=QImage(width,height,QImage::Format_RGB16);if(frame.isNull())User::Leave(KErrNoMemory);for(TUint y=0;y<height;y++)Mem::Copy(frame.scanLine(y),raw.constData()+y*width*2,width*2);
        }
        view->setPixmap(QPixmap::fromImage(frame.scaled(view->size(),Qt::KeepAspectRatio,Qt::FastTransformation)));
    }
    QLabel* view;QLabel* note;bool web;int timer;TUint width,height,count,num,den;qint64 activeMs,lastPreviewMs;QElapsedTimer previewClock;QFile file;WebContent content;
};
static void SywpPreview(const QString& name){SywpPreviewDialog dialog;TRAPD(error,dialog.OpenL(name));if(error){QMessageBox::information(0,"BelleWall",SywpError(error));return;}dialog.showMaximized();dialog.exec();logLine("UI preview closed");}
static QChar SywpPickImportDrive(){
    RFs fs;if(fs.Connect())return QChar();QStringList choices;QList<QChar> drives;const QChar preferred=SywpImportDrive();int current=0;
    for(const char* candidate="CEF";*candidate;candidate++){
        const TInt drive=*candidate=='C'?EDriveC:*candidate=='E'?EDriveE:EDriveF;
        TVolumeInfo volume;if(fs.Volume(volume,drive)!=KErrNone)continue;
        QChar letter(*candidate);if(letter==preferred)current=choices.size();drives<<letter;
        choices<<BwText("%1 盘 · 可用 %2 MiB").arg(letter).arg(double(volume.iFree)/1048576.0,0,'f',1);
    }
    fs.Close();if(choices.isEmpty())return QChar();bool ok=false;
    QString choice=QInputDialog::getItem(0,BwText("壁纸存储位置"),BwText("导入副本保存到哪个盘？原始 SYWP 文件不移动。"),choices,current,false,&ok);
    return ok?drives[choices.indexOf(choice)]:QChar();
}
static QString SywpManageLibrary(){
    for(;;){int action=0;QString name=SywpPickWallpaper(SywpChosen(),action);if(action<=0)return QString();
        if(action==3){QString file=QFileDialog::getOpenFileName(0,BwText("导入 SYWP 壁纸包"),"E:/",BwText("壁纸包 (*.sywp)"));if(file.isEmpty())continue;QChar drive=SywpPickImportDrive();if(drive.isNull())continue;TRAPD(error,SywpImportL(file,drive));logLine(QString("UI library import drive=%1 result=%2").arg(drive).arg(error));if(error)QMessageBox::information(0,"BelleWall",SywpError(error));continue;}
        if(name.isEmpty())continue;
        if(action==2){SywpConfirmDelete(name);continue;}if(action==4){SywpPreview(name);continue;}
        TRAPD(error,SywpSelectL(name,name.endsWith(".html")));if(error){QMessageBox::information(0,"BelleWall",SywpError(error));continue;}return name;
    }
}
#include "preparationui.h"
#include "maintenanceui.h"
class WallpaperSettingsDialog:public QDialog {
public:
    WallpaperSettingsDialog():QDialog(),status(new QLabel(this)),selection(new QLabel(this)),loggedState(-1){
        setWindowTitle("BelleWall 1.1");QVBoxLayout* outer=new QVBoxLayout(this);QScrollArea* scroll=new QScrollArea(this);scroll->setWidgetResizable(true);QWidget* body=new QWidget(scroll);scroll->setWidget(body);outer->addWidget(scroll);QVBoxLayout* layout=new QVBoxLayout(body);QLabel* heading=new QLabel(BwText("BelleWall"),this);QFont font=heading->font();font.setPointSize(20);heading->setFont(font);layout->addWidget(heading);
        status->setWordWrap(true);selection->setWordWrap(true);selection->setTextFormat(Qt::PlainText);layout->addWidget(status);layout->addWidget(selection);
        QStringList actions=QStringList()<<BwText("壁纸管理")<<BwText("恢复桌面")<<BwText("运行诊断")<<BwText("检查并准备组件")<<BwText("卸载准备")<<BwText("返回桌面")<<BwText("语言 / Language");const int ids[]={2,4,7,8,9,6,10};QSignalMapper* mapper=new QSignalMapper(this);
        for(int i=0;i<actions.size();i++){buttons[i]=new QPushButton(actions[i],this);buttons[i]->setMinimumHeight(42);layout->addWidget(buttons[i]);QObject::connect(buttons[i],SIGNAL(clicked()),mapper,SLOT(map()));mapper->setMapping(buttons[i],ids[i]);}
        QObject::connect(mapper,SIGNAL(mapped(int)),this,SLOT(done(int)));QLabel* note=new QLabel(BwText("BelleWall 1.1\n播放或恢复时请保持解锁。"),this);note->setWordWrap(true);layout->addWidget(note);refresh();startTimer(500);
    }
protected:
    void timerEvent(QTimerEvent*){refresh();}
private:
    void refresh(){
        RChunk chunk;TCandidateShared* shared=CandidateOpen(chunk);bool active=false;TUint state=0;
        if(shared){RProcess owner;if(owner.Open(TProcessId(shared->record.owner))==KErrNone){active=owner.SecureId().iId==TInt(0xe7b31103)&&owner.ExitType()==EExitPending;owner.Close();}if(active)state=shared->record.state;}chunk.Close();
        const bool pending=QFile::exists("C:/data/BelleWall/candidate-session.bin")||QFile::exists("C:/data/BelleWall/pages-rollback.bin")||QFile::exists("C:/data/BelleWall/wallpaper-rollback.bin");
        RMutex lock;TInt opened=lock.OpenGlobal(_L("BelleWallWallpaperExperiment"));if(opened==KErrNone)lock.Close();const bool busy=opened!=KErrNotFound;
        const int displayState=active?int(state):(busy?7:pending?8:0);if(displayState!=loggedState){loggedState=displayState;CEikonEnv* env=CEikonEnv::Static();logLine(QString("UI observed_state=%1 own_group=%2 focus_group=%3").arg(displayState).arg(env?env->RootWin().Identifier():-1).arg(env?env->WsSession().GetFocusWindowGroup():-1));}
        QString text;
        if(active){switch(state){case EPreparing:case EBinding:text=BwText("正在准备壁纸，请回到桌面并保持解锁。");break;case ERunning:text=BwText("壁纸正在运行。");break;case EPaused:text=BwText("壁纸已暂停；返回亮屏桌面并使用壁纸允许的方向后继续。");break;default:text=BwText("正在停止并恢复；请回到桌面并保持解锁。");break;}}
        else if(busy)text=BwText("正在处理壁纸，请稍候；恢复时请回到桌面。");
        else if(pending)text=BwText("桌面恢复尚未完成。请点「恢复桌面」，恢复前无法播放或导入。");
        else {text=BwText("已停止 · 桌面可正常使用");QFile outcome("C:/data/BelleWall/candidate-last-result.txt");if(outcome.open(QIODevice::ReadOnly)&&outcome.size()<=24){bool ok=false;int error=outcome.readAll().toInt(&ok);if(ok&&error<0)text=BwText("上次操作未完成（错误码 %1）。当前没有待恢复记录，可重试；若持续失败，请保存日志反馈。").arg(error);}}
        if(!active&&!busy&&!pending&&QFile::exists("C:/data/BelleWall/preparation-pending.bin"))text=BwText("上次组件操作未完成。请点「检查并准备组件」重试，或执行「卸载准备」；请勿手动删除记录。");
        if(active)text+=BwText("\n动态桌面运行期间，壁纸管理不可用。要更换壁纸，请先停止并恢复桌面。");
        status->setText(text);QString chosen=SywpChosen();if(chosen!=lastChosen){lastChosen=chosen;lastLabel=chosen.isEmpty()?QString():SywpLabel(chosen);}
        selection->setText(chosen.isEmpty()?BwText("还没有选择壁纸，请先导入 .sywp 文件。"):BwText(active?"当前播放内容：":"已选中，尚未播放：")+lastLabel);
        const bool idle=!active&&!pending&&!busy;buttons[0]->setEnabled(idle);buttons[1]->setEnabled(active||(!busy&&pending));buttons[1]->setText(BwText(active?"停止并恢复桌面":"恢复桌面"));buttons[2]->setEnabled(true);buttons[3]->setEnabled(idle);buttons[4]->setEnabled(idle);

    }
    QLabel* status;QLabel* selection;QPushButton* buttons[7];QString lastChosen,lastLabel;int loggedState;
};
static bool SywpLiveSession(){RChunk chunk;TCandidateShared* shared=CandidateOpen(chunk);bool active=false;if(shared){RProcess owner;if(owner.Open(TProcessId(shared->record.owner))==KErrNone){active=owner.SecureId().iId==TInt(0xe7b31103)&&owner.ExitType()==EExitPending;owner.Close();}}chunk.Close();return active;}
static void WallpaperSettings(){
    QDir().mkpath("C:/data/BelleWall");
    for(;;){int action;{WallpaperSettingsDialog dialog;dialog.showMaximized();action=dialog.exec();}logLine(QString("UI dashboard action=%1").arg(action));
        if(action<=0)return;
        if(action==6){TRAPD(home,CandidateShowDesktopL());if(home)QMessageBox::information(0,"BelleWall",BwText("请手动返回原生桌面。"));return;}
        if(action==10){QStringList choices;choices<<BwText("跟随系统")<<QString::fromUtf8("中文")<<"English";bool ok=false;QString chosen=QInputDialog::getItem(0,BwText("语言 / Language"),BwText("选择界面语言"),choices,0,false,&ok);if(ok){int index=choices.indexOf(chosen);TRAPD(saved,SywpAtomicL("C:/data/BelleWall/ui-language.txt",index==1?QByteArray("zh"):index==2?QByteArray("en"):QByteArray("auto")));if(!saved)BwResetLanguage();}continue;}
        if(action>=7){WallpaperMaintenance(action);continue;}
        QString command;
        if(action==2){QString chosen=SywpManageLibrary();if(chosen.isEmpty())continue;
            QStringList durations=QStringList()<<BwText("60 秒检查")<<BwText("10 分钟")<<BwText("持续播放，直到手动停止");bool ok=false;
            QString duration=QInputDialog::getItem(0,BwText("播放时长"),BwText("持续模式请通过恢复桌面按钮结束"),durations,0,false,&ok);if(!ok)continue;
            TInt error=PrepareWallpaper();if(error){QMessageBox::information(0,"BelleWall",PreparationError(error));continue;}
            command=chosen.endsWith(".html")?"--candidate-web":"--candidate-video";if(duration==durations[1])command+=" --600";else if(duration==durations[2])command+=" --continuous";
        }else command=SywpLiveSession()?"--candidate-stop":"--candidate-recover";
        TRAPD(error,CandidateLaunchL(command));if(!error)return;QMessageBox::information(0,"BelleWall",SywpError(error));
    }
}
#endif
