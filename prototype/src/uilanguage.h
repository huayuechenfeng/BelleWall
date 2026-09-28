#ifndef BELLEWALL_UI_LANGUAGE_H
#define BELLEWALL_UI_LANGUAGE_H
#include <QtCore/QLocale>
#include <QtCore/QFile>
#include <cstring>
#include "ui_strings.h"
static int bwLanguage=-1;
static void BwResetLanguage(){bwLanguage=-1;}
static bool BwEnglish(){if(bwLanguage<0){QFile file("C:/data/BelleWall/ui-language.txt");QByteArray preference;if(file.open(QIODevice::ReadOnly)&&file.size()<16)preference=file.readAll();bwLanguage=preference=="en"?1:preference=="zh"?0:QLocale::system().language()==QLocale::Chinese?0:1;}return bwLanguage==1;}
static QString BwText(const char* text){if(BwEnglish())for(unsigned i=0;i<sizeof(KBwTranslations)/sizeof(KBwTranslations[0]);i++)if(!std::strcmp(text,KBwTranslations[i][0]))return QString::fromUtf8(KBwTranslations[i][1]);return QString::fromUtf8(text);}
#endif
