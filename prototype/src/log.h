#ifndef BELLEWALL_LOG_H
#define BELLEWALL_LOG_H
#include <QtCore/QFile>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <e32debug.h>
inline void logLine(const QString& message) {
    QDir().mkpath("C:/data/BelleWall");
    QFile f("C:/data/BelleWall/host.log");
    if (f.size()>512*1024) { QFile::remove("C:/data/BelleWall/host.previous.log"); if(!f.rename("C:/data/BelleWall/host.previous.log"))return; f.setFileName("C:/data/BelleWall/host.log"); }
    if(f.open(QIODevice::WriteOnly|QIODevice::Append)) f.write((QDateTime::currentDateTime().toString(Qt::ISODate)+" "+message+"\r\n").toUtf8());
    const QString shortMessage=message.left(220);
    const TPtrC text(reinterpret_cast<const TUint16*>(shortMessage.utf16()),shortMessage.size());
    RDebug::Print(_L("BelleWall: %S"),&text);
}
#endif
