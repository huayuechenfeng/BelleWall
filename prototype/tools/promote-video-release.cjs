'use strict';
// Promote only the verified 1.0.2 components. This changes the outer package
// identity shown to users; the signed inner SIS files remain byte-for-byte.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..');
const source=path.resolve(process.argv[2]||path.join(root,'build/checkpoints/BelleWall-1.0.2-video-candidate-v2'));
const output=path.resolve(process.argv[3]||path.join(root,'dist/release/1.0.2'));
const work=path.join(root,'build/release-checkpoints/BelleWall-1.0.2');
if(fs.existsSync(output)||fs.existsSync(work))throw Error('Release output or checkpoint exists; choose a clean path');
const candidate=JSON.parse(fs.readFileSync(path.join(source,'validation.json'),'utf8'));
if(candidate.version!=='1.0.2'||candidate.installer!=='BelleWall-1.0.2-video-candidate.sisx')throw Error('Expected the verified 1.0.2 video candidate');
const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
if(sha(path.join(source,candidate.installer))!==candidate.sha256)throw Error('Candidate installer hash changed');
const names=['bellerender-selfsigned.sisx','bellepaper-selfsigned.sisx','bellewall-selfsigned.sisx'];
const uids=['e7b31106','e7b31103','e7b31101'];
for(let i=0;i<names.length;i++){
 const item=candidate.components.find(x=>x.name===names[i]);
 if(!item||item.uid.toLowerCase()!=='0x'+uids[i]||sha(path.join(source,names[i]))!==item.sha256)throw Error('Candidate component hash or UID changed: '+names[i]);
}
fs.mkdirSync(output,{recursive:true});fs.mkdirSync(work,{recursive:true});
const sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474';
function run(exe,args,label){const result=cp.spawnSync(exe,args,{encoding:'utf8',windowsHide:true,maxBuffer:32*1024*1024});fs.writeFileSync(path.join(work,label+'.log'),(result.stdout||'')+(result.stderr||''));if(result.error||result.status)throw Error(label+': '+(result.error||result.stderr||result.stdout));}
const pkg=path.join(work,'BelleWall-1.0.2.pkg'),unsigned=path.join(work,'BelleWall-1.0.2-unsigned.sis'),installer=path.join(output,'BelleWall-1.0.2.sisx');
fs.writeFileSync(pkg,'&EN\n#{"BelleWall 1.0.2"},(0xE7B31109),1,0,2\n%{"BelleWall"}\n:"BelleWall"\n'+names.map((name,i)=>'@"'+path.join(source,name).replaceAll('\\','/')+'",(0x'+uids[i]+')').join('\n')+'\n');
run(sdk+'/epoc32/tools/makesis.exe',[pkg,unsigned],'makesis');
run(sdk+'/epoc32/tools/signsis.exe',['-s',unsigned,installer,path.join(root,'build/signing/prototype.cer'),path.join(root,'build/signing/prototype.key')],'signsis');
run(sdk+'/epoc32/tools/signsis.exe',['-o',installer],'signature-check');
const extracted=path.join(work,'extracted');fs.mkdirSync(extracted);
run(sdk+'/epoc32/tools/dumpsis.exe',['-x','-d',extracted,installer],'extract');
const manifestFile=path.join(extracted,'bellewall-1.0.2.pkg');
const bytes=fs.readFileSync(manifestFile),manifest=bytes.toString(bytes[0]===0xff?'utf16le':'utf8');
const header=manifest.match(/^#\{([^\r\n]+)\},\s*\(0x([0-9a-f]+)\),\s*(\d+),\s*(\d+),\s*(\d+),\s*TYPE=SA$/mi);
const order=[...manifest.matchAll(/^@"sis(\d+)\.sis",\(0x([0-9a-f]+)\)$/gmi)].map(x=>[Number(x[1]),x[2].toLowerCase()]);
if(!header||!header[1].includes('BelleWall 1.0.2')||header[2].toLowerCase()!=='e7b31109'||header.slice(3).join('.')!=='1.0.2'||JSON.stringify(order)!==JSON.stringify(uids.map((uid,i)=>[i,uid])))throw Error('Release package identity, version or component order mismatch');
const reconstructed=path.join(work,'combined-verify');fs.mkdirSync(reconstructed);
for(let i=0;i<names.length;i++)fs.copyFileSync(path.join(extracted,'sis'+i+'.sis'),path.join(reconstructed,names[i]));
const inputs=path.join(reconstructed,'package-inputs');fs.mkdirSync(inputs);
for(const name of fs.readdirSync(path.join(source,'package-inputs')))fs.copyFileSync(path.join(source,'package-inputs',name),path.join(inputs,name));
const result=cp.spawnSync(process.execPath,[path.join(root,'prototype/tools/verify-product-packages.cjs'),reconstructed,path.join(work,'payload-verification.json')],{cwd:root,env:{...process.env,BELLEWALL_PRODUCT_VERSION:'1.0.2'},encoding:'utf8',windowsHide:true,maxBuffer:32*1024*1024});
fs.writeFileSync(path.join(work,'payload-verification.log'),(result.stdout||'')+(result.stderr||''));
if(result.error||result.status)throw Error('Release payload verification failed: '+(result.error||result.stderr||result.stdout));
const report={channel:'release',version:'1.0.2',installer:path.basename(installer),sha256:sha(installer),sourceCandidate:{installer:candidate.installer,sha256:candidate.sha256},components:candidate.components,offlineChecks:['signature','nested UID and version','component order','payload identity and dependencies'],deviceEvidence:'E7: user reports current 1.0.2 test is very good; detailed frame-rate/log evidence was not supplied',pending:['603 video and drive tests','E7 landscape switch crashes','E6 and other nonstandard screens','English UI']};
fs.writeFileSync(path.join(output,'validation.json'),JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify({installer,sha256:report.sha256,work},null,2));
