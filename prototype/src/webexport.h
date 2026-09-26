#ifndef BELLEWALL_WEBEXPORT_H
#define BELLEWALL_WEBEXPORT_H
#include "softwarevideo.h"
// Demand-driven, phone-side WebKit rendering for the native wallpaper adapter.
class WebExport:public QObject {
public:
    WebExport(bool video=false):last(-1),videoMode(video),busy(false){elapsed.start();idle.start();}
    void startL(){if(videoMode)video.loadL();else content.loadL("C:/data/BelleWall/animation.html");startTimer(100);logLine(videoMode?"VIDEO EXPORT ready":"WEB EXPORT ready");}
protected:
    void timerEvent(QTimerEvent*){
        if(busy)return;
        if(elapsed.elapsed()>60000||QFile::exists("C:/data/BelleWall/web-stop")){qApp->quit();return;}
        QFile request("C:/data/BelleWall/web-request");
        if(!request.open(QIODevice::ReadOnly)){if(idle.elapsed()>1500)content.pause();return;}
        bool ok=false;int frame=request.read(16).trimmed().toInt(&ok);request.close();
        if(!ok||frame<0||frame>=20||frame==last){if(idle.elapsed()>1500)content.pause();return;}
        busy=true;TRAPD(error,renderL(frame));busy=false;
        if(error){logLine(QString("WEB EXPORT failed=%1").arg(error));qApp->exit(error);return;}
        last=frame;idle.restart();logLine(QString(videoMode?"VIDEO EXPORT frame=%1":"WEB EXPORT frame=%1").arg(frame));
    }
private:
    void renderL(int frame){
        QImage image;
        if(videoMode)image=video.frameL(frame);else {
        content.resumeL(QSize(180,320));
        // Pump Qt while asynchronous HTML loading finishes; never export a loading frame.
        for(int n=0;n<100&&!content.ready();n++){QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,20);User::After(10000);}
        if(!content.ready())User::Leave(KErrTimedOut);
        image=content.frameL(1000);}
        const QString path=QString("C:/data/BelleWall/native-frame-%1.bmp").arg(frame);
        if(!image.save(path,"BMP"))User::Leave(KErrWrite);
        QFile ack("C:/data/BelleWall/web-ready");if(!ack.open(QIODevice::WriteOnly|QIODevice::Truncate))User::Leave(KErrWrite);
        QByteArray value=QByteArray::number(frame);if(ack.write(value)!=value.size()||!ack.flush())User::Leave(KErrWrite);
    }
    WebContent content;SoftwareVideo video;QElapsedTimer elapsed,idle;int last;bool videoMode,busy;
};
#endif
