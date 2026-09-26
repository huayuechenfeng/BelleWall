'use strict';
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
const root=path.resolve(__dirname,'../..'),dist=path.join(root,'dist'),sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474';
const out=path.join(root,'build/sis-check'),evidence=path.join(root,'research/evidence/prototype');fs.mkdirSync(out,{recursive:true});
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
for(const line of fs.readFileSync(path.join(dist,'SHA256SUMS.txt'),'utf8').trim().split('\n')){const [h,name]=line.split('  ');if(sha(path.join(dist,name))!==h)throw Error('Artifact hash differs: '+name);}
const result=cp.spawnSync(sdk+'/epoc32/tools/dumpsis.exe',['-x','-d',out,path.join(dist,'bellewall-selfsigned.sisx')],{encoding:'utf8'});if(result.error||result.status)throw Error(result.error||result.stdout+result.stderr);
const names=['bellewall.exe','bellewall.rsc','bellewall_reg.rsc','config.ini','animation.html','sample.mp4','THIRD-PARTY.txt','belleweb.exe','belleweb.rsc','belleweb_reg.rsc'];
const payloads=names.map((n,i)=>({name:n,hash:sha(path.join(dist,n)),extractedHash:sha(path.join(out,'file'+i))}));
if(payloads.some(x=>x.hash!==x.extractedHash))throw Error('SIS extracted payload mismatch');
if(fs.readFileSync(path.join(out,'file0')).readUInt32LE(8)!==0xe7b31101||fs.readFileSync(path.join(out,'file7')).readUInt32LE(8)!==0xe7b31130)throw Error('Packaged UI and worker application identities are incorrect');
const caps=cp.spawnSync(sdk+'/epoc32/tools/dumpsis.exe',['-l',path.join(dist,'bellewall-selfsigned.sisx')],{encoding:'utf8'});if(caps.status||!caps.stdout.includes('Executable1: capabilities matched')||!caps.stdout.includes('Executable8: capabilities matched'))throw Error('SIS executable capability check failed');
fs.writeFileSync(path.join(evidence,'sis-capabilities.txt'),caps.stdout);
fs.writeFileSync(path.join(evidence,'delivery-check.json'),JSON.stringify({sisSha256:sha(path.join(dist,'bellewall-selfsigned.sisx')),payloads,capabilities:'EXE matches SIS capabilities. Old dumpsis also labels non-executable payloads as executables; those files are not E32 executables.',scope:'Static package validation; device results are recorded separately in research/evidence/device/SESSION.md'},null,2));
console.log('All 10 extracted SIS payloads match; app and worker capabilities match; artifact hashes match.');
