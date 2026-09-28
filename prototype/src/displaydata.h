#ifndef BELLEWALL_DISPLAY_DATA_H
#define BELLEWALL_DISPLAY_DATA_H
#include "displaypolicy.h"
struct TContentDisplay {
    TUint magic,width,height,fit,orientation,background,reserved;
    TContentDisplay():magic(0x31445742),width(180),height(320),fit(0),orientation(0),background(0),reserved(0){}
};
typedef char TContentDisplaySizeCheck[sizeof(TContentDisplay)==28?1:-1];
static TContentDisplay ReadContentDisplayL(RFs& files){
    TContentDisplay policy;RFile selection;TInt err=selection.Open(files,_L("C:\\data\\BelleWall\\selected-wallpaper.txt"),EFileRead|EFileShareReadersOnly);if(err==KErrNotFound)return policy;User::LeaveIfError(err);CleanupClosePushL(selection);
    TBuf8<80> id;User::LeaveIfError(selection.Read(id));CleanupStack::PopAndDestroy(&selection);TInt prefix=id.Length()==70||id.Length()==71?2:0;
    if(id.Length()!=68+prefix&&id.Length()!=69+prefix)User::Leave(KErrCorrupt);
    if(prefix&&(id[1]!=':'||(id[0]!='C'&&id[0]!='E'&&id[0]!='F')))User::Leave(KErrCorrupt);
    for(TInt i=prefix;i<prefix+64;i++)if(!((id[i]>='0'&&id[i]<='9')||(id[i]>='a'&&id[i]<='f')))User::Leave(KErrCorrupt);
    if(id.Mid(prefix+64)!=_L8(".bwv")&&id.Mid(prefix+64)!=_L8(".mp4")&&id.Mid(prefix+64)!=_L8(".html"))User::Leave(KErrCorrupt);
    TFileName path(_L("C:\\data\\BelleWall\\library\\"));if(prefix)path[0]=id[0];TBuf<80> name;name.Copy(id.Mid(prefix));path.Append(name);path.Append(_L(".display"));
    RFile file;err=file.Open(files,path,EFileRead|EFileShareReadersOnly);if(err==KErrNotFound)return policy;User::LeaveIfError(err);CleanupClosePushL(file);TInt size;User::LeaveIfError(file.Size(size));TPtr8 bytes(reinterpret_cast<TUint8*>(&policy),sizeof(policy),sizeof(policy));User::LeaveIfError(file.Read(bytes));CleanupStack::PopAndDestroy(&file);
    if(size!=sizeof(policy)||bytes.Length()!=sizeof(policy)||policy.magic!=0x31445742||!BelleDisplay::Valid(policy.width,policy.height)||policy.fit>2||policy.orientation>2||policy.background>0xffffff||policy.reserved)User::Leave(KErrCorrupt);return policy;
}
#endif
