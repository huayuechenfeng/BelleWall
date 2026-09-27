#ifndef BELLEWALL_SYWP_STORAGE_H
#define BELLEWALL_SYWP_STORAGE_H
#include <QtCore/QString>
#include <QtCore/QRegExp>

// An old selection without a drive belongs to the original C: library.
static QString SywpStorageId(const QString& value){
    if(QRegExp("[0-9a-f]{64}\\.(bwv|mp4|html)").exactMatch(value))return "C:"+value;
    if(QRegExp("[CEF]:[0-9a-f]{64}\\.(bwv|mp4|html)").exactMatch(value))return value;
    return QString();
}
static QString SywpStoragePath(const QString& value){
    const QString id=SywpStorageId(value);
    return id.isEmpty()?QString():id.left(2)+"/data/BelleWall/library/"+id.mid(2);
}
#endif
