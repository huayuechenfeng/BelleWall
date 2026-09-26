#include "host.h"
#include "diagnostics.h"
#include "webexport.h"
#include "webstream.h"
#include "sywpimport.h"
int main(int argc,char** argv) {
    QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    const bool worker=RProcess().SecureId().iId==0xe7b31130;
    if(worker&&!app.arguments().contains("--export-stream")&&!app.arguments().contains("--export-web")&&!app.arguments().contains("--export-video"))return KErrArgument;
    if(app.arguments().size()==3&&app.arguments().at(1)=="--import-sywp"){TRAPD(error,SywpImportL(app.arguments().at(2)));logLine(QString("SYWP import result=%1 selected=%2 temporaryFiles=%3").arg(error).arg(SywpChosen()).arg(QDir("C:/data/BelleWall/library").entryList(QStringList()<<"*.tmp",QDir::Files).size()));return error;}
    if(app.arguments().size()==1||app.arguments().contains("--settings")){WallpaperSettings();return 0;}
    if(CEikonEnv::Static()) { RWindowGroup& group=CEikonEnv::Static()->RootWin();group.EnableReceiptOfFocus(EFalse);group.SetOrdinalPosition(-1,-1000); }
    QDir().mkpath("C:/data/BelleWall");
    const QStringList args=app.arguments();
    if(args.contains("--export-stream")){WebStream stream;TRAPD(err,stream.startL());if(err)logLine(QString("STREAM start failed=%1").arg(err));return err?err:app.exec();}
    if(args.contains("--export-web")||args.contains("--export-video")){WebExport exporter(args.contains("--export-video"));TRAPD(err,exporter.startL());if(err)logLine(QString("EXPORT start failed=%1").arg(err));return err?err:app.exec();}
    if(args.contains("--diagnose")){TRAPD(err,diagnoseL());return err;}
    if(args.contains("--stop")){QFile f("C:/data/BelleWall/stop");if(!f.open(QIODevice::WriteOnly)){logLine("STOP marker write failed");return KErrAccessDenied;}logLine("STOP requested");return 0;}
    RMutex mutex;TInt lockError=mutex.CreateGlobal(_L("BelleWallPrototypeHost"));
    if(lockError!=KErrNone){logLine(QString("Singleton=%1; not launching duplicate").arg(lockError));return lockError;}
    QSettings c("C:/data/BelleWall/config.ini",QSettings::IniFormat);
    QString mode;
    if(args.contains("--blocks"))mode="blocks";if(args.contains("--video"))mode="video";if(args.contains("--web"))mode="web";
    if(args.contains("--legacy-blocks"))mode="blocks";
    c.setValue("experimentalLegacy",bool(args.contains("--legacy-blocks")));
    if(!mode.isEmpty()){c.setValue("mode",mode);if(mode=="video")c.setValue("file","C:/data/BelleWall/sample.mp4");else if(mode=="web")c.setValue("file","C:/data/BelleWall/animation.html");else c.setValue("file","");}
    c.setValue("enabled",true);c.sync();if(c.status()!=QSettings::NoError){logLine("Config write failed");mutex.Close();return KErrWrite;}QFile::remove("C:/data/BelleWall/stop");
    Host* host=new Host;TRAPD(err,host->startL());
    if(err)logLine(QString("START FAILED=%1").arg(err));
    const int result=err?err:app.exec();delete host;mutex.Close();return result;
}
