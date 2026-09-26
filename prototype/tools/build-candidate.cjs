'use strict';
// This historical pipeline would combine current workers with obsolete package
// metadata and renderer identity. Refuse before writing any build/output files.
throw Error('Historical 0.2 pipeline is frozen. Use build-product.cjs with a new output directory; reproduce 0.2 only from its archived source.');
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||path.join(root,'dist/daily-candidate-20260920'));
const input=process.argv[3]||'C:/Users/chihoko/Downloads/3690417937.mpkg';
const resume=process.argv.includes('--resume');
if(fs.existsSync(out)&&!resume)throw Error('Checkpoint exists; choose a new output directory');
if(resume&&fs.existsSync(path.join(out,'validation.json')))throw Error('Completed checkpoint cannot be resumed');
fs.mkdirSync(out,{recursive:true});const evidence=path.join(out,'evidence');fs.mkdirSync(evidence,{recursive:true});
const sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474',dist=path.join(root,'dist');
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
function run(exe,args,label){const r=cp.spawnSync(exe,args,{cwd:root,encoding:'utf8',maxBuffer:32*1024*1024,windowsHide:true});fs.writeFileSync(path.join(evidence,label+'.log'),JSON.stringify({exe,args})+'\n'+(r.stdout||'')+(r.stderr||''));if(r.error||r.status)throw Error(label+' failed: '+(r.error||r.status));console.log(label+' passed');}
function node(tool,args=[]){run(process.execPath,[path.join(root,'prototype/tools',tool),...args],tool.replace('.cjs',''));}
function verifyHashes(dir){const lines=fs.readFileSync(path.join(dir,'SHA256SUMS.txt'),'utf8').trim().split(/\r?\n/);for(const line of lines){const split=line.indexOf('  ');if(split!==64||sha(path.join(dir,line.slice(split+2)))!==line.slice(0,split))throw Error('Checkpoint hash mismatch: '+line);}return lines.length;}
const baseline=path.join(dist,'background-dirty-pages-20260919'),baselineCount=verifyHashes(baseline);
if(!resume){run(process.execPath,['--test','prototype/tests/mpkg.test.cjs','prototype/tests/sywp.test.cjs','prototype/tests/device-session.test.cjs'],'offline-tests');
node('make-background-profile.cjs');node('build-native-wallpaper.cjs');node('build-render-probe.cjs');node('build.cjs');node('package.cjs');node('check-delivery.cjs');}
const built=JSON.parse(fs.readFileSync(path.join(root,'build/arm/success.json')));for(const [name,hash] of Object.entries(built.sources))if(sha(path.join(root,name))!==hash)throw Error('Source no longer matches build: '+name);
const assets=path.join(out,'content');if(!resume)node('verify-sywp-conversion.cjs',[input,assets]);
const sywp=require('./sywp.cjs');const clock=fs.readFileSync(path.join(root,'prototype/content/clock.html'));
fs.writeFileSync(path.join(assets,'clock.sywp'),sywp.encode({format:'sywp',version:1,title:'真实时钟与轻量动画',kind:'web',width:180,height:320,loop:true,pause:'resume',entry:'index.html',display:sywp.display},clock));
const pkg=path.join(assets,'candidate-content.pkg');fs.writeFileSync(pkg,'&EN\n#{"BelleWall Candidate Content"},(0xE7B31105),0,2,0\n%{"BelleWall"}\n:"BelleWall"\n"'+path.join(assets,'video-frames.bin').replaceAll('\\','/')+'"-"C:\\data\\BelleWall\\video-frames.bin"\n');
run(sdk+'/epoc32/tools/makesis.exe',[pkg,path.join(assets,'content.sis')],'content-makesis');
run(sdk+'/epoc32/tools/signsis.exe',['-s',path.join(assets,'content.sis'),path.join(out,'bellecontent-selfsigned.sisx'),path.join(root,'build/signing/prototype.cer'),path.join(root,'build/signing/prototype.key')],'content-sign');
const artifacts=['bellepaper.exe','bellepaper-selfsigned.sisx','bellerenderhost.exe','bellerendercandidate.dll','bellerendercandidate.rsc','bellerender-selfsigned.sisx','bellewall.exe','bellewall.rsc','bellewall_reg.rsc','bellewall-selfsigned.sisx'];
for(const n of artifacts)fs.copyFileSync(path.join(dist,n),path.join(out,n));
const groups=[['bellepaper-selfsigned.sisx',[path.join(out,'bellepaper.exe')]],['bellerender-selfsigned.sisx',['bellerendercandidate.dll','bellerendercandidate.rsc','bellerenderhost.exe'].map(n=>path.join(out,n))],['bellecontent-selfsigned.sisx',[path.join(assets,'video-frames.bin')]]];
const packages=[];for(const [sis,expected] of groups){const extracted=path.join(evidence,sis+'-extracted');fs.mkdirSync(extracted,{recursive:true});run(sdk+'/epoc32/tools/dumpsis.exe',['-x','-d',extracted,path.join(out,sis)],sis+'-payloads');const files=expected.map((file,i)=>{if(sha(file)!==sha(path.join(extracted,'file'+i)))throw Error('SIS payload mismatch '+sis);return {file:path.basename(file),sha256:sha(file)};});packages.push({sis,files});}
fs.writeFileSync(path.join(evidence,'sis-payloads.json'),JSON.stringify(packages,null,2));
for(const [from,to] of [['build/arm/build.log','qt-command-log.txt'],['build/native-wallpaper/build.log','native-command-log.txt'],['build/render-probe/build.log','render-command-log.txt'],['build/arm/success.json','qt-build-success.json'],['research/evidence/prototype/delivery-check.json','qt-sis-payloads.json']])fs.copyFileSync(path.join(root,from),path.join(evidence,to));
function copyTree(from,to){fs.mkdirSync(to,{recursive:true});for(const e of fs.readdirSync(from,{withFileTypes:true})){if(e.isSymbolicLink())throw Error('Unexpected source symlink');if(e.isDirectory())copyTree(path.join(from,e.name),path.join(to,e.name));else fs.copyFileSync(path.join(from,e.name),path.join(to,e.name));}}
copyTree(path.join(root,'prototype'),path.join(out,'source/prototype'));
for(const rel of ['research/sources/homescreen-s3/idlehomescreen/sapiwrapper/hspswrapper/inc','research/sources/homescreen-s3/idlehomescreen/inc'])copyTree(path.join(root,rel),path.join(out,'source',rel));
fs.copyFileSync(path.join(root,'README.md'),path.join(out,'source/README.md'));
verifyHashes(baseline);
const validation={version:'0.2.0-offline-candidate',date:'2026-09-20',targetFirmware:'Nokia 603 RM-779 Belle FP2 113.010.1506; four default black pages',status:'offline-built-device-pending',baseline:{path:'../background-dirty-pages-20260919',verifiedFiles:baselineCount,unchanged:true},capabilities:{armBuild:'passed',sisPayloadHashes:'passed',pcPackageAndHttpTests:'passed',realVideoMpKgConversion:'passed: 30s/300 frames plus 360x640@30 and 640x480@20 fixtures',nativeLifecycle:'implemented; device pending',sessionIsolation:'implemented; device fault injection pending',journalRecovery:'implemented; interruption/conflict/device pending',phoneSywpImport:'compiled; device pending',dirtyVideo:'device pending',realtimeWebClock:'device pending',tenMinuteRun:'not run',screenLockAndResume:'device pending',rotationPlayback:'unsupported by candidate',e6Playback:'unsupported by candidate',automaticBootRecovery:'not installed',physicalLcdFps:'not measured',power:'not measured',endToEndFbsReadSynchronization:'not proven'},files:[...artifacts,'bellecontent-selfsigned.sisx'].map(name=>({name,sha256:sha(path.join(out,name))}))};
fs.writeFileSync(path.join(out,'validation.json'),JSON.stringify(validation,null,2));
fs.writeFileSync(path.join(out,'README.md'),'# BelleWall 0.2 离线候选\n\n尚未实机验收，不能宣布日用通过。当前四个SISX与源码对应；逐项状态见 validation.json。操作、恢复、安装与测试矩阵见 [候选说明](source/doc/development/CANDIDATE-20260920.md)，格式见 [SYWP规范](source/doc/SYWP-FORMAT.md)。先安装三个程序包，再装内容包；旧版本恢复和新元数据注册步骤不得省略。content/case-0.sywp 是完整30秒10fps视频，clock.sywp 是实时网页。case-1/2 是高帧率/横屏格式测试素材，不能据此宣称手机支持。\n\n源码含第三方解码器及构建所需项目头文件；不含SDK、ROM、FFmpeg、签名私钥。构建使用外部SDK与工作区研究指纹来源，见BUILD.md。没有连接设备。\n');
function walk(dir){return fs.readdirSync(dir,{withFileTypes:true}).flatMap(e=>e.isDirectory()?walk(path.join(dir,e.name)):[path.join(dir,e.name)]);}
const files=walk(out).sort();fs.writeFileSync(path.join(out,'SHA256SUMS.txt'),files.map(p=>sha(p)+'  '+path.relative(out,p).replaceAll('\\','/')).join('\n')+'\n');verifyHashes(out);console.log('Checkpoint verified: '+out+' ('+files.length+' files)');
