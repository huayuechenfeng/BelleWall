'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),dist=path.join(root,'dist');
const sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474',openssl=process.env.OPENSSL||'C:/Program Files/Git/mingw64/bin/openssl.exe';
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const built=JSON.parse(fs.readFileSync(path.join(root,'build/arm/success.json')));
for(const [n,h]of Object.entries(built.sources))if(sha(path.join(root,n))!==h)throw Error('Source changed after build: '+n);
if(sha(path.join(dist,'bellewall.exe'))!==built.exeSha256)throw Error('EXE differs from successful build');
if(sha(path.join(dist,'belleweb.exe'))!==built.workerSha256)throw Error('Worker differs from successful build');
for(const [n,h]of Object.entries(built.resources))if(sha(path.join(dist,n))!==h)throw Error('Resource differs from successful build');
function run(exe,args){const r=cp.spawnSync(exe,args,{cwd:dist,encoding:'utf8'});if(r.error)throw r.error;if(r.status)throw Error(r.stdout+'\n'+r.stderr);return r.stdout;}
for(const [source,name]of [['prototype/content/clock.html','animation.html'],['prototype/config/config.ini','config.ini'],[process.env.BELLEWALL_DEMO_MP4||'build/prepared-video/sample.mp4','sample.mp4']])fs.copyFileSync(path.resolve(root,source),path.join(dist,name));
fs.writeFileSync(path.join(dist,'THIRD-PARTY.txt'),['PROVENANCE.txt','LICENSE.md','Apache-2.0.txt'].map(n=>fs.readFileSync(path.join(root,'prototype/vendor/h264bsd',n),'utf8')).join('\n\n'));
const pkg=`; No ROM or theme files are installed or changed.
&EN
#{"BelleWall"},(0xE7B31101),1,0,0
%{"BelleWall Research"}
:"BelleWall Research"
(0xE7B31103),1,0,0,{"BelleWall Native"}
"bellewall.exe"-"C:\\sys\\bin\\bellewall.exe"
"bellewall.rsc"-"C:\\resource\\apps\\bellewall.rsc"
"bellewall_reg.rsc"-"C:\\private\\10003a3f\\import\\apps\\bellewall_reg.rsc"
"config.ini"-"C:\\data\\BelleWall\\config.ini"
"animation.html"-"C:\\data\\BelleWall\\animation.html"
"sample.mp4"-"C:\\data\\BelleWall\\sample.mp4"
"THIRD-PARTY.txt"-"C:\\data\\BelleWall\\THIRD-PARTY.txt"
"belleweb.exe"-"C:\\sys\\bin\\belleweb.exe"
"belleweb.rsc"-"C:\\resource\\apps\\belleweb.rsc"
"belleweb_reg.rsc"-"C:\\private\\10003a3f\\import\\apps\\belleweb_reg.rsc"
`;
fs.writeFileSync(path.join(dist,'bellewall.pkg'),pkg);
run(sdk+'/epoc32/tools/makesis.exe',['bellewall.pkg','bellewall-unsigned.sis']);
const signing=path.join(root,'build/signing');fs.mkdirSync(signing,{recursive:true});
const cert=path.join(signing,'prototype.cer'),key=path.join(signing,'prototype.key');
if(!fs.existsSync(cert)||!fs.existsSync(key))run(openssl,['req','-new','-newkey','rsa:2048','-nodes','-x509','-sha1','-days','3650','-subj','/CN=BelleWall Development Only/O=Local Prototype','-keyout',key,'-out',cert]);
run(sdk+'/epoc32/tools/signsis.exe',['-s','bellewall-unsigned.sis','bellewall-selfsigned.sisx',cert,key]);
const verification=run(sdk+'/epoc32/tools/signsis.exe',['-o','bellewall-selfsigned.sisx']);
fs.writeFileSync(path.join(dist,'signature-info.txt'),verification);
const files=['bellewall.exe','bellewall.rsc','bellewall_reg.rsc','belleweb.exe','belleweb.rsc','belleweb_reg.rsc','bellewall-unsigned.sis','bellewall-selfsigned.sisx','sample.mp4','animation.html','config.ini','THIRD-PARTY.txt','bellepaper.exe','bellepaper-selfsigned.sisx'];
fs.writeFileSync(path.join(dist,'SHA256SUMS.txt'),files.map(n=>crypto.createHash('sha256').update(fs.readFileSync(path.join(dist,n))).digest('hex')+'  '+n).join('\n')+'\n');
console.log('Packaged and signature inspected: '+path.join(dist,'bellewall-selfsigned.sisx'));
