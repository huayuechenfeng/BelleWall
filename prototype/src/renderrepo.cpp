// Read-only SID-policy diagnostic; no GUI environment and no setters.
#include <e32base.h>
#include <f32file.h>
static void Trace(const TDesC& event){RFs fs;if(fs.Connect())return;fs.MkDirAll(_L("C:\\data\\BelleWall\\"));RFile file;if(file.Open(fs,_L("C:\\data\\BelleWall\\render-plugin.log"),EFileWrite|EFileShareAny)&&file.Create(fs,_L("C:\\data\\BelleWall\\render-plugin.log"),EFileWrite|EFileShareAny)){fs.Close();return;}TInt pos=0;file.Seek(ESeekEnd,pos);TBuf8<200> line;line.Copy(event.Left(180));line.Append(_L8("\r\n"));file.Write(line);file.Flush();file.Close();fs.Close();}
#include "definitionprobe.h"
GLDEF_C TInt E32Main(){CTrapCleanup* cleanup=CTrapCleanup::New();if(!cleanup)return KErrNoMemory;CActiveScheduler* scheduler=new CActiveScheduler;if(!scheduler){delete cleanup;return KErrNoMemory;}CActiveScheduler::Install(scheduler);TRAPD(error,InspectRenderDefinitionL());TBuf<80> line;line.Format(_L("DEF SID probe result=%d"),error);Trace(line);delete scheduler;delete cleanup;return error;}
