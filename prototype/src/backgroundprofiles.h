#ifndef BELLE_BACKGROUND_PROFILES_H
#define BELLE_BACKGROUND_PROFILES_H
#include <e32std.h>
#include "backgroundprofile.h"

struct TBackgroundLayout {
    TInt id;
    TUint viewAddress;
    TUint backgroundOffset;
    TUint vtableAddress;
    TUint drawAddress;
};

static const TBackgroundLayout K603Layout={603,0x993c,0x5c,0x40740,0x3231d};
// E7-00 RM-626 / 111.040.1511; xn3layoutengine.dll SHA-256
// f2203cbd0e83f7fdc7e69959be32aeea93a1063c8ef2a2cd42feb6e034900c54.
static const TBackgroundLayout KE7Layout={7,0x98c4,0x5c,0x40394,0x32079};
static const TUint8 KE7View[]={0x80,0x30,0x10,0xb5,0x40,0x69,0x80,0x69,0xc0,0x46,0x10,0xbd};
static const TUint8 KE7Create[]={0xa0,0x6d,0xe6,0xf7,0xe9,0xfc,0x29,0x00,0x0e,0xf0,0xb9,0xfc,0xe0,0x65,0xa0,0x6d,0x0d,0xf0,0xb7,0xff};
static const TUint8 KE7Construct[]={0xd6,0xf7,0x80,0xec,0xe4,0x49,0x01,0x60,0xc8,0x31,0x41,0x60,0x18,0x31,0x41,0x63,0x06,0x64,0x18,0x31,0xc5,0x63,0x04,0x00,0x81,0x63,0x44,0x30};
static const TUint8 KE7Draw[]={0xf3,0xb5,0x04,0x00,0x91,0xb0,0xd6,0xf7,0x3c,0xe9,0x0f,0x90,0x20,0x00,0xd6,0xf7,0xdc,0xe8,0x05,0x00,0x00,0x68,0x12,0x99};

static TBool MatchE7BackgroundCode(TUint base){
    return Mem::Compare(reinterpret_cast<const TUint8*>(base+0x98c4),sizeof(KE7View),KE7View,sizeof(KE7View))==0&&
        Mem::Compare(reinterpret_cast<const TUint8*>(base+0x23026),sizeof(KE7Create),KE7Create,sizeof(KE7Create))==0&&
        Mem::Compare(reinterpret_cast<const TUint8*>(base+0x3194e),sizeof(KE7Construct),KE7Construct,sizeof(KE7Construct))==0&&
        Mem::Compare(reinterpret_cast<const TUint8*>(base+0x32078),sizeof(KE7Draw),KE7Draw,sizeof(KE7Draw))==0;
}

// Ref7-style masked anchors retain instruction semantics while ignoring BL
// displacements and literal-pool distances that vary between firmware builds.
// The exported View() entry and ordinal 277 bound the contiguous code scan.
static const TUint8 KGenericCreate[]={0x20,0x68,0,0xf0,0,0,0x29,0,0,0xf0,0,0,0x20,0x60,0x20,0x68};
static const TUint8 KGenericCreateMask[]={0x3f,0xf8,0,0xf8,0,0,0xff,0xff,0,0xf8,0,0,0x3f,0xf8,0x3f,0xf8};
static const TUint8 KGenericFactory[]={0x70,0xb5,0x04,0x00,0x0d,0x00,0xb4,0x20};
static const TUint8 KGenericConstructorBody[]={0x01,0x60,0xc8,0x31,0x41,0x60,0x18,0x31,0x41,0x63,0x06,0x64,0x18,0x31,0xc5,0x63,0x04,0x00,0x81,0x63,0x44,0x30};
static const TUint8 KGenericDraw[]={0xf3,0xb5,0x04,0x00,0x91,0xb0,0,0,0,0,0x0f,0x90,0x20,0x00,0,0,0,0,0x05,0x00,0x00,0x68,0x12,0x99};
static const TUint8 KGenericDrawMask[]={0xff,0xff,0xff,0xff,0xff,0xff,0,0,0,0,0xff,0xff,0xff,0xff,0,0,0,0,0xff,0xff,0xff,0xff,0xff,0xff};
static TBool BackgroundAddressInRange(TUint at,TUint length,TUint begin,TUint end){
    return at>=begin&&at<=end&&length<=end-at;
}
static TBool MatchBackgroundMasked(TUint at,const TUint8* expected,const TUint8* mask,TInt length){
    const TUint8* actual=reinterpret_cast<const TUint8*>(at);
    for(TInt i=0;i<length;i++)if((actual[i]&mask[i])!=expected[i])return EFalse;
    return ETrue;
}
static TBool DecodeBackgroundBl(TUint at,TUint& target){
    const TUint16 first=*reinterpret_cast<const TUint16*>(at);
    const TUint16 second=*reinterpret_cast<const TUint16*>(at+2);
    if((first&0xf800)!=0xf000||(second&0xd000)!=0xd000)return EFalse;
    const TUint s=(first>>10)&1,j1=(second>>13)&1,j2=(second>>11)&1;
    const TUint i1=~(j1^s)&1,i2=~(j2^s)&1;
    TUint delta=(s<<24)|(i1<<23)|(i2<<22)|((first&0x3ff)<<12)|((second&0x7ff)<<1);
    if(s)delta|=0xfe000000u;
    target=at+4+delta;return ETrue;
}
static TBool GenericBackgroundCandidate(TUint create,TUint begin,TUint end,TBackgroundLayout& found){
    TUint helper=0;if(!DecodeBackgroundBl(create+2,helper)||!BackgroundAddressInRange(helper,4,begin,end))return EFalse;
    const TUint16 load=*reinterpret_cast<const TUint16*>(create);
    const TUint16 store=*reinterpret_cast<const TUint16*>(create+12);
    const TUint16 reload=*reinterpret_cast<const TUint16*>(create+14);
    if((load&0xf83f)!=0x6820||(store&0xf83f)!=0x6020||(reload&0xf83f)!=0x6820)return EFalse;
    const TUint offset=((store>>6)&31)*4;
    if(offset<4||offset!=(((load>>6)&31)*4)+4||((reload>>6)&31)!=((load>>6)&31))return EFalse;
    TUint factory=0;if(!DecodeBackgroundBl(create+8,factory)||!BackgroundAddressInRange(factory,24,begin,end))return EFalse;
    if(Mem::Compare(reinterpret_cast<const TUint8*>(factory),sizeof(KGenericFactory),KGenericFactory,sizeof(KGenericFactory))!=0)return EFalse;
    TUint constructorEntry=0;if(!DecodeBackgroundBl(factory+16,constructorEntry))return EFalse;
    const TUint constructor=constructorEntry+6;
    if(!BackgroundAddressInRange(constructor,6+sizeof(KGenericConstructorBody),begin,end))return EFalse;
    if(Mem::Compare(reinterpret_cast<const TUint8*>(constructor+6),sizeof(KGenericConstructorBody),KGenericConstructorBody,sizeof(KGenericConstructorBody))!=0)return EFalse;
    const TUint16 literalLoad=*reinterpret_cast<const TUint16*>(constructor+4);
    if((literalLoad&0xff00)!=0x4900)return EFalse; // Thumb LDR r1, [pc, #imm]
    const TUint literal=((constructor+8)&~3u)+((literalLoad&0xff)*4);
    if(!BackgroundAddressInRange(literal,4,begin,end))return EFalse;
    const TUint vtable=*reinterpret_cast<const TUint*>(literal);
    if((vtable&3)||!BackgroundAddressInRange(vtable,0xa8,begin,end))return EFalse;
    const TUint draw=*reinterpret_cast<const TUint*>(vtable+0xa4);
    if(!(draw&1)||!BackgroundAddressInRange(draw&~1u,sizeof(KGenericDraw),begin,end))return EFalse;
    if(!MatchBackgroundMasked(draw&~1u,KGenericDraw,KGenericDrawMask,sizeof(KGenericDraw)))return EFalse;
    found.id=0;found.viewAddress=begin;found.backgroundOffset=offset;found.vtableAddress=vtable;found.drawAddress=draw;
    return ETrue;
}
static TBool FindGenericBackgroundLayout(TUint begin,TUint end,TBackgroundLayout& found){
    if(end<=begin||end-begin<0x1000||end-begin>=0x80000)return EFalse;
    if(Mem::Compare(reinterpret_cast<const TUint8*>(begin),sizeof(KBgFingerprint0),KBgFingerprint0,sizeof(KBgFingerprint0))!=0)return EFalse;
    TInt count=0;
    for(TUint at=begin;at<=end-sizeof(KGenericCreate);at+=2){
        const TUint8* bytes=reinterpret_cast<const TUint8*>(at);
        if((bytes[0]&0x3f)!=0x20||(bytes[1]&0xf8)!=0x68)continue;
        if(!MatchBackgroundMasked(at,KGenericCreate,KGenericCreateMask,sizeof(KGenericCreate)))continue;
        TBackgroundLayout candidate;
        if(!GenericBackgroundCandidate(at,begin,end,candidate))continue;
        if(++count>1)return EFalse;
        found=candidate;
    }
    return count==1;
}

// A single binary selects an audited layout by loaded code signatures. Ordinal
// 277 is beyond the code sites and vtable draw slot in both audited images, so
// it witnesses a mapped range before those addresses are read. Unknown layouts fail
// before dereferencing the private desktop object.
static TBool FindBackgroundLayout(RLibrary& library,TLibraryFunction view,TUint& base,TBackgroundLayout& result){
    base=0;
    TLibraryFunction end=library.Lookup(277);
    if(!end)return EFalse;
    const TUint address=reinterpret_cast<TUint>(view)&~1u;
    const TUint mapped=reinterpret_cast<TUint>(end)&~1u;
    if(address>=K603Layout.viewAddress){
        const TUint candidate=address-K603Layout.viewAddress;
        if(mapped>=candidate&&mapped-candidate>=K603Layout.vtableAddress+0xa4+4&&mapped-candidate<0x80000&&
           MatchBackgroundCode(candidate)){base=candidate;result=K603Layout;return ETrue;}
    }
    if(address>=KE7Layout.viewAddress){
        const TUint candidate=address-KE7Layout.viewAddress;
        if(mapped>=candidate&&mapped-candidate>=KE7Layout.vtableAddress+0xa4+4&&mapped-candidate<0x80000&&
           MatchE7BackgroundCode(candidate)){base=candidate;result=KE7Layout;return ETrue;}
    }
    return FindGenericBackgroundLayout(address,mapped,result);
}
#endif
