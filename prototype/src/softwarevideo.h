#ifndef BELLEWALL_SOFTWAREVIDEO_H
#define BELLEWALL_SOFTWAREVIDEO_H
extern "C" {
#include "../vendor/h264bsd/src/h264bsd_decoder.h"
}
// Deliberately narrow MP4 reader: one video track, avc1 Baseline, one chunk,
// constant 15 fps, four-byte NAL lengths. Matches the shipped import profile.
// All box/sample boundaries are checked before passing bytes to the decoder.
class SoftwareVideo {
    struct Box {int p,n;Box(int a=0,int b=0):p(a),n(b){} int end()const{return p+n;} };
public:
    SoftwareVideo():active(false),offset(0),decoded(-1),width(0),height(0),left(0),top(0),stride(0),rows(0){}
    ~SoftwareVideo(){if(active)h264bsdShutdown(&decoder);}
    void loadL(){
        QFile f("C:/data/BelleWall/sample.mp4");if(!f.open(QIODevice::ReadOnly)||f.size()>4*1024*1024)User::Leave(KErrArgument);mp4=f.readAll();
        Box root(0,mp4.size()),moov=findL(root,"moov",0),mdat=findL(root,"mdat",0),trak=findL(moov,"trak"),mdia=findL(trak,"mdia"),hdlr=findL(mdia,"hdlr");
        require(hdlr.n>=20&&mp4.mid(hdlr.p+16,4)=="vide");
        Box stbl=findL(findL(mdia,"minf"),"stbl"),stsd=findL(stbl,"stsd"),stsz=findL(stbl,"stsz"),stsc=findL(stbl,"stsc"),stco=findL(stbl,"stco"),stts=findL(stbl,"stts"),mdhd=findL(mdia,"mdhd");
        require(stsd.n>=16&&be(stsd.p+12)==1);Box avc1=findL(stsd,"avc1",16);require(avc1.n>=86);Box avcc=findL(avc1,"avcC",86);
        require(avcc.n>=15&&byte(avcc.p+8)==1&&byte(avcc.p+9)==66&&(byte(avcc.p+12)&3)==3);
        int p=avcc.p+14;int sps=byte(avcc.p+13)&31;require(sps>0);
        for(int i=0;i<sps;i++)parameterL(p,avcc.end());require(p<avcc.end());int pps=byte(p++);require(pps>0);for(int i=0;i<pps;i++)parameterL(p,avcc.end());require(p==avcc.end());
        require(stsz.n>=20&&be(stsz.p+12)==0);int count=be(stsz.p+16);require(count>0&&count<=900&&stsz.n==20+count*4);
        require(stco.n==20&&be(stco.p+12)==1&&be(stco.p+16)==unsigned(mdat.p+8));
        require(stsc.n==28&&be(stsc.p+12)==1&&be(stsc.p+16)==1&&be(stsc.p+20)==unsigned(count)&&be(stsc.p+24)==1);
        require(mdhd.n>=32&&byte(mdhd.p+8)==0&&stts.n==24&&be(stts.p+12)==1&&be(stts.p+16)==unsigned(count));
        require(be(mdhd.p+20)>0&&quint64(be(stts.p+20))*15==be(mdhd.p+20));
        p=mdat.p+8;
        for(int i=0;i<count;i++){
            unsigned size=be(stsz.p+20+i*4);require(size>0&&size<=unsigned(mdat.end()-p));int end=p+size;
            while(p<end){require(end-p>=4);unsigned n=be(p);p+=4;require(n>0&&n<=unsigned(end-p));nal(p,n);p+=n;}
        }
        require(p==mdat.end());mp4.clear();resetL();logLine("SOFTWARE VIDEO MP4 validated; decoder on phone, 15 fps source");
    }
    QImage frameL(int second){
        const int wanted=second*15;
        while(decoded<wanted)nextL();
        return picture;
    }
private:
    static void require(bool ok){if(!ok)User::Leave(KErrNotSupported);}
    unsigned byte(int p)const{require(p>=0&&p<mp4.size());return static_cast<unsigned char>(mp4[p]);}
    unsigned be(int p)const{require(p>=0&&p<=mp4.size()-4);return(byte(p)<<24)|(byte(p+1)<<16)|(byte(p+2)<<8)|byte(p+3);}
    Box findL(Box parent,const char* type,int header=8){
        Box result;int p=parent.p+header;require(p<=parent.end());
        while(p<parent.end()){require(parent.end()-p>=8);unsigned n=be(p);require(n>=8&&n<=unsigned(parent.end()-p));if(mp4.mid(p+4,4)==type){require(!result.n);result=Box(p,n);}p+=n;}
        require(result.n>0);return result;
    }
    void nal(int p,int n){annex.append("\0\0\0\1",4);annex.append(mp4.constData()+p,n);}
    void parameterL(int& p,int end){require(end-p>=2);int n=(byte(p)<<8)|byte(p+1);p+=2;require(n>0&&n<=end-p);nal(p,n);p+=n;}
    void resetL(){if(active)h264bsdShutdown(&decoder);active=false;Mem::FillZ(&decoder,sizeof(decoder));if(h264bsdInit(&decoder,1))User::Leave(KErrNoMemory);active=true;working=annex;working.detach();offset=0;}
    static int clip(int n){return n<0?0:n>255?255:n;}
    void nextL(){
        for(int attempts=0;attempts<4096;attempts++){
            if(offset>=working.size())resetL();
            u32 used=0;u32 result=h264bsdDecode(&decoder,reinterpret_cast<u8*>(working.data())+offset,working.size()-offset,0,&used);
            require(used<=unsigned(working.size()-offset));offset+=used;
            if(result==H264BSD_HDRS_RDY){u32 crop=0;h264bsdCroppingParams(&decoder,&crop,&left,&width,&top,&height);stride=h264bsdPicWidth(&decoder)*16;rows=h264bsdPicHeight(&decoder)*16;if(!crop){left=top=0;width=stride;height=rows;}require(width==180&&height==320&&stride<=192&&rows<=320&&left+width<=stride&&top+height<=rows);}
            else if(result==H264BSD_PIC_RDY){u32 id,idr,errors;u8* yuv=h264bsdNextOutputPicture(&decoder,&id,&idr,&errors);if(!yuv)continue;require(!errors&&width&&height);
                picture=QImage(width,height,QImage::Format_RGB32);if(picture.isNull())User::Leave(KErrNoMemory);
                int plane=stride*rows;
                for(unsigned y=0;y<height;y++){QRgb* row=reinterpret_cast<QRgb*>(picture.scanLine(y));for(unsigned x=0;x<width;x++){int sx=x+left,sy=y+top,Y=int(yuv[sy*stride+sx])-16,U=int(yuv[plane+(sy/2)*(stride/2)+sx/2])-128,V=int(yuv[plane+plane/4+(sy/2)*(stride/2)+sx/2])-128;row[x]=qRgb(clip((298*Y+409*V+128)>>8),clip((298*Y-100*U-208*V+128)>>8),clip((298*Y+516*U+128)>>8));}}
                ++decoded;return;
            } else if(result!=H264BSD_RDY)User::Leave(KErrCorrupt);
        }
        User::Leave(KErrCorrupt);
    }
    QByteArray mp4,annex,working;storage_t decoder;bool active;int offset,decoded;u32 width,height,left,top,stride,rows;QImage picture;
};
#endif
