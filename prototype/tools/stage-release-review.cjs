'use strict';
// Local review only: reuse verified phone packages; never publish or rebuild.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..');
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const json=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const state=json(path.join(root,'prototype/project-state.json'));
const checkpoint=path.resolve(root,state.installedCheckpoint);
const out=process.argv[2]&&path.resolve(process.argv[2]);
if(!out||fs.existsSync(out))throw Error('Supply a new, nonexistent review directory');
for(const line of fs.readFileSync(path.join(checkpoint,'SHA256SUMS.txt'),'utf8').trim().split(/\r?\n/)){
 const file=path.resolve(checkpoint,line.slice(66));
 if(!file.startsWith(checkpoint+path.sep)||sha(file)!==line.slice(0,64))throw Error('Checkpoint hash mismatch: '+line.slice(66));
}
const entries=[];
function copy(source,target){const dest=path.join(out,target);fs.mkdirSync(path.dirname(dest),{recursive:true});fs.copyFileSync(source,dest);entries.push({file:target,source:path.relative(root,source).replaceAll('\\','/'),sha256:sha(dest)});}
function write(target,value){const dest=path.join(out,target);fs.mkdirSync(path.dirname(dest),{recursive:true});fs.writeFileSync(dest,value);entries.push({file:target,source:'generated',sha256:sha(dest)});}
for(const name of ['bellerender-selfsigned.sisx','bellepaper-selfsigned.sisx','bellewall-selfsigned.sisx'])copy(path.join(checkpoint,name),'phone/'+name);
for(const kind of ['video','web'])copy(path.join(checkpoint,'examples',kind+'.sywp'),'examples/BelleWall-'+kind+'.sywp');
for(const name of ['README.md','INSTALL.md','KNOWN-ISSUES.md','FEEDBACK.md','CONTENT-NOTICES.md'])copy(path.join(root,'doc/release',name),name);
copy(path.join(root,'doc/release/PC-QUICKSTART.md'),'pc/README.md');
for(const name of ['sywp-webui.cjs','sywp-webui.html','sywp.cjs','prepare-sywp.cjs','mpkg.cjs'])copy(path.join(checkpoint,'source/prototype/tools',name),'pc/prototype/tools/'+name);
copy(path.join(checkpoint,'source/prototype/content/clock.html'),'pc/prototype/content/clock.html');
copy(path.join(root,'doc/SYWP-FORMAT.md'),'docs/SYWP-FORMAT.md');
copy(path.join(checkpoint,'source/prototype/content/clock.html'),'docs/content/clock.html');
copy(path.join(checkpoint,'package-inputs/THIRD-PARTY.txt'),'notices/h264bsd.txt');
write('pc/Start-BelleWall.cmd','@echo off\r\nnode "%~dp0prototype\\tools\\sywp-webui.cjs"\r\nif errorlevel 1 pause\r\n');
const evidence=[];
for(const key of ['installedEvidence','faultRecoveryEvidence','web30MinuteEvidence','video30MinuteEvidence','importWorkflowEvidence'])if(state[key]){
 const source=path.join(root,state[key]),data=json(source);if(data.candidate!==state.installedCheckpoint)throw Error('Evidence checkpoint differs: '+key);
 if(key!=='installedEvidence'&&data.status!=='passed')throw Error('Device evidence has not passed: '+key);
 copy(source,'evidence/'+key+'.json');evidence.push({key,file:'evidence/'+key+'.json',status:data.status||'see evidence'});
}
const manifest={status:'local-review-not-published',publicVersion:'0.1',sisVersion:state.sisVersion,checkpoint:state.installedCheckpoint,phonePackagesRebuilt:false,evidence,enduranceScope:state.enduranceScope,pending:[...(!state.importWorkflowEvidence?['Phone import rejection checks and final PC-to-phone workflow']:[]),'Project distribution license and dependency/source notices review','Final publication decision'],files:entries};
write('review-manifest.json',JSON.stringify(manifest,null,2)+'\n');
fs.writeFileSync(path.join(out,'SHA256SUMS.txt'),entries.slice().sort((a,b)=>a.file.localeCompare(b.file)).map(e=>e.sha256+'  '+e.file).join('\n')+'\n');
console.log(JSON.stringify({directory:out,status:manifest.status,files:entries.length,checkpoint:state.installedCheckpoint},null,2));
