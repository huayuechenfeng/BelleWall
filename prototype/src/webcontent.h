#ifndef BELLEWALL_WEBCONTENT_H
#define BELLEWALL_WEBCONTENT_H
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebSettings>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtGui/QPainter>
#include "log.h"
// The prototype accepts a self-contained document. No subresource networking.
class OfflineNetwork : public QNetworkAccessManager {
protected:
    QNetworkReply* createRequest(Operation op,const QNetworkRequest&,QIODevice*) {
        return QNetworkAccessManager::createRequest(op,QNetworkRequest(QUrl("data:text/plain,")),0);
    }
};
class LocalPage : public QWebPage {
protected:
    bool acceptNavigationRequest(QWebFrame*,const QNetworkRequest& r,NavigationType) {
        return r.url().scheme()=="about" || r.url().scheme()=="data";
    }
};
class WebContent {
public:
    WebContent():page(0),clockMs(0),loadWaitMs(0),loaded(false),resizePending(false) {}
    ~WebContent(){pause();}
    void loadL(const QString& path) {
        QFile f(path);
        if(!f.open(QIODevice::ReadOnly)||f.size()>256*1024) User::Leave(KErrArgument);
        html=QString::fromUtf8(f.readAll());
        if(!html.contains("bellewallStep")) User::Leave(KErrNotSupported);
        clockMs=0;
    }
    void resumeL(const QSize& size) {
        if(page){if(page->viewportSize()!=size){page->setViewportSize(size);image=QImage(size,QImage::Format_RGB16);if(image.isNull())User::Leave(KErrNoMemory);resizePending=true;}return;}
        page=new LocalPage;
        OfflineNetwork* net=new OfflineNetwork; net->setParent(page); page->setNetworkAccessManager(net);
        page->settings()->setAttribute(QWebSettings::PluginsEnabled,false);
        page->settings()->setAttribute(QWebSettings::JavaEnabled,false);
        page->settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls,false);
        page->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls,false);
        page->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows,false);
        page->setViewportSize(size);resizePending=true;
        page->mainFrame()->setScrollBarPolicy(Qt::Horizontal,Qt::ScrollBarAlwaysOff);
        page->mainFrame()->setScrollBarPolicy(Qt::Vertical,Qt::ScrollBarAlwaysOff);
        page->mainFrame()->setHtml(html,QUrl("about:blank"));
        // setHtml can complete asynchronously on the device WebKit build.
        loaded=false;loadWaitMs=0;
        image=QImage(size,QImage::Format_RGB16);
        if(image.isNull())User::Leave(KErrNoMemory);
    }
    bool ready(){if(page&&!loaded)loaded=page->mainFrame()->evaluateJavaScript("typeof bellewallStep === 'function'").toBool();return page&&loaded;}
    bool responsive(){return ready()&&page->mainFrame()->evaluateJavaScript("typeof bellewallResize === 'function'").toBool();}
    void pause(){delete page;page=0;image=QImage();}
    const QImage& frameL(int delta) {
        if(!page)User::Leave(KErrNotReady);
        if(!loaded){
            loaded=page->mainFrame()->evaluateJavaScript("typeof bellewallStep === 'function'").toBool();
            if(!loaded){loadWaitMs+=delta;if(loadWaitMs>5000)User::Leave(KErrTimedOut);image.fill(0);return image;}
        }
        if(resizePending){const QString resize=QString("(function(){try{if(typeof bellewallResize === 'function')bellewallResize(%1,%2);return true;}catch(e){return false;}})()").arg(image.width()).arg(image.height());if(!page->mainFrame()->evaluateJavaScript(resize).toBool())User::Leave(KErrCorrupt);resizePending=false;}
        clockMs+=delta;
        const QString step=QString("(function(){try{bellewallStep(%1);return true;}catch(e){return false;}})()").arg(clockMs);
        if(!page->mainFrame()->evaluateJavaScript(step).toBool())User::Leave(KErrCorrupt);
        image.fill(0);QPainter painter(&image);page->mainFrame()->render(&painter);return image;
    }
private:
    LocalPage* page;QString html;QImage image;qint64 clockMs;int loadWaitMs;bool loaded,resizePending;
};
#endif
