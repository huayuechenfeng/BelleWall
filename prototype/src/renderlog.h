#ifndef BELLEWALL_RENDER_LOG_H
#define BELLEWALL_RENDER_LOG_H
// Never wait for another writer in the desktop UI thread. An exclusive open
// either succeeds immediately or drops this diagnostic line. Two bounded files.
static void BoundedRenderTrace(const TDesC& path,const TDesC& previous,const TDesC& event){
    RFs fs;if(fs.Connect())return;fs.MkDirAll(_L("C:\\data\\BelleWall\\"));RFile file;
    TInt opened=file.Open(fs,path,EFileWrite|EFileShareExclusive);if(opened==KErrNotFound)opened=file.Create(fs,path,EFileWrite|EFileShareExclusive);
    if(opened){fs.Close();return;}TInt size=0;if(file.Size(size)){file.Close();fs.Close();return;}
    TBuf8<200> line;line.Copy(event.Left(180));line.Append(_L8("\r\n"));
    if(size>256*1024-line.Length()){
        file.Close();TInt removed=fs.Delete(previous);if(removed!=KErrNone&&removed!=KErrNotFound){fs.Close();return;}
        if(fs.Rename(path,previous)||file.Create(fs,path,EFileWrite|EFileShareExclusive)){fs.Close();return;}size=0;
    }
    if(file.Seek(ESeekEnd,size)==KErrNone)file.Write(line);file.Close();fs.Close();
}
#endif
