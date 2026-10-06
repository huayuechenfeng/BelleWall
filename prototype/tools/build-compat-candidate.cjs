'use strict';
// One paired upgrade package for offline-verified desktop layouts. Device
// acceptance is separate; this script never installs or starts a phone app.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..');
const kind=process.env.BELLEWALL_CANDIDATE_KIND==='video'?'video':'compat';
const version=process.env.BELLEWALL_CANDIDATE_VERSION||(kind==='video'?'1.0.2':'1.0.1');
if(!/^\d+\.\d+\.\d+$/.test(version))throw Error('Invalid candidate version');
const displayCandidate=Number(version.split('.')[0])>1||Number(version.split('.')[1])>=1;
const sisVersion=version.replaceAll('.',',');
const release=process.env.BELLEWALL_CHANNEL==='release';
const baseName='BelleWall-'+version+(release?'':'-'+kind+'-candidate');
const out=path.resolve(process.argv[2]||path.join(root,'build/checkpoints',baseName));
const renderer=path.join(out,'renderer-build');
if(fs.existsSync(out))throw Error('Candidate output exists; choose a new path');
fs.mkdirSync(out,{recursive:true});
const sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474';
const env={...process.env,BELLEWALL_PRODUCT_VERSION:version};
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
function run(exe,args,label){
    const result=cp.spawnSync(exe,args,{cwd:root,env,encoding:'utf8',windowsHide:true,maxBuffer:32*1024*1024});
    fs.writeFileSync(path.join(out,label+'.log'),(result.stdout||'')+(result.stderr||''));
    if(result.error||result.status)throw Error(label+': '+(result.error||result.stderr||result.stdout));
    console.log(label+' passed');
}
const node=(script,args=[],label=path.basename(script))=>run(process.execPath,[script,...args],label);
node('prototype/tools/verify-background-profiles.cjs',[],'rom-profile-verification');
node('prototype/tools/analyze-background-generic.cjs',[],'generic-layout-verification');
node('prototype/tools/build-render-probe.cjs',['--longrun','--out',renderer],'renderer-build');
node('prototype/tools/build-render-probe.cjs',['--host-only'],'helper-build');
node('prototype/tools/package-session-helper.cjs',[renderer],'helper-package');
node('prototype/tools/build-native-wallpaper.cjs',[],'native-package');
node('prototype/tools/build.cjs',[],'qt-build');
node('prototype/tools/package.cjs',[],'qt-package');
const dist=path.join(root,'dist');
node('prototype/tools/verify-product-packages.cjs',['dist',path.join(out,'paired-validation.json')],'paired-verification');
const components=[['bellerender-selfsigned.sisx','0xE7B31106'],['bellepaper-selfsigned.sisx','0xE7B31103'],['bellewall-selfsigned.sisx','0xE7B31101']];
for(const [name] of components)fs.copyFileSync(path.join(dist,name),path.join(out,name));
const inputs=path.join(out,'package-inputs');fs.mkdirSync(inputs);
for(const name of ['bellewall.mif','bellerendercandidate.dll','bellerendercandidate.rsc','bellerenderhost.exe','bellerenderlongrun.dll','bellerenderlongrun.rsc','bellepaper.exe','bellewall.exe','bellewall.rsc','bellewall_reg.rsc','config.ini','animation.html','sample.mp4','THIRD-PARTY.txt','belleweb.exe','belleweb.rsc','belleweb_reg.rsc'])fs.copyFileSync(path.join(dist,name),path.join(inputs,name));
const spec='&EN\n#{"BelleWall '+version+(release?'':' '+(kind==='video'?'video':'compatibility')+' candidate')+'"},(0xE7B31109),'+sisVersion+'\n%{"BelleWall"}\n:"BelleWall"\n'+components.map(([name,uid])=>'@"'+path.join(out,name).replaceAll('\\','/')+'",('+uid+')').join('\n')+'\n';
const pkg=path.join(out,'BelleWall-'+version+'.pkg'),unsigned=path.join(out,'BelleWall-'+version+'-unsigned.sis'),installer=path.join(out,baseName+'.sisx');
fs.writeFileSync(pkg,spec);
run(sdk+'/epoc32/tools/makesis.exe',[pkg,unsigned],'combined-makesis');
run(sdk+'/epoc32/tools/signsis.exe',['-s',unsigned,installer,path.join(root,'build/signing/prototype.cer'),path.join(root,'build/signing/prototype.key')],'combined-sign');
run(sdk+'/epoc32/tools/signsis.exe',['-o',installer],'combined-signature');
const extracted=path.join(out,'combined-extracted');fs.mkdirSync(extracted);
run(sdk+'/epoc32/tools/dumpsis.exe',['-x','-d',extracted,installer],'combined-extract');
const manifestFile=path.join(extracted,path.basename(installer,'.sisx').toLowerCase()+'.pkg');
const manifestBytes=fs.readFileSync(manifestFile),manifest=manifestBytes.toString(manifestBytes[0]===0xff?'utf16le':'utf8');
const header=manifest.match(/^#\{[^\r\n]+\},\s*\(0x([0-9a-f]+)\),\s*(\d+),\s*(\d+),\s*(\d+),\s*TYPE=SA$/mi);
const order=[...manifest.matchAll(/^@"sis(\d+)\.sis",\(0x([0-9a-f]+)\)$/gmi)].map(x=>[Number(x[1]),x[2].toLowerCase()]);
if(!header||header[1].toLowerCase()!=='e7b31109'||header.slice(2).join('.')!==version||JSON.stringify(order)!==JSON.stringify([[0,'e7b31106'],[1,'e7b31103'],[2,'e7b31101']]))throw Error('Combined package identity, version or order mismatch');
const reconstructed=path.join(out,'combined-verify');fs.mkdirSync(reconstructed);
for(let i=0;i<components.length;i++)fs.copyFileSync(path.join(extracted,'sis'+i+'.sis'),path.join(reconstructed,components[i][0]));
fs.mkdirSync(path.join(reconstructed,'package-inputs'));
for(const name of fs.readdirSync(inputs))fs.copyFileSync(path.join(inputs,name),path.join(reconstructed,'package-inputs',name));
node('prototype/tools/verify-product-packages.cjs',[reconstructed,path.join(out,'combined-validation.json')],'combined-verification');
const samples=[];
if(kind==='video'){
 const source=path.join(out,'test-pattern-30fps.mp4');
 run(process.env.BELLEWALL_TEST_FFMPEG||'ffmpeg',['-nostdin','-v','error','-f','lavfi','-i','testsrc2=size=360x640:rate=30:duration=2','-an','-c:v','mpeg4','-q:v','4','-bf','0','-pix_fmt','yuv420p','-movflags','+faststart',source],'sample-source');
 const {prepare}=require('./prepare-sywp.cjs'),{read}=require('./sywp.cjs');
 for(const fps of [20,30]){const file=path.join(out,`test-${fps}fps-360x640.sywp`);prepare(source,file,{title:`BelleWall ${fps} fps 测试`,width:360,height:640,fps,frames:fps*2});const parsed=read(file);if(parsed.manifest.fpsNumerator!==fps||parsed.manifest.width!==360||parsed.manifest.height!==640||parsed.manifest.frames!==fps*2)throw Error('Video test package mismatch');samples.push({name:path.basename(file),fps,width:360,height:640,frames:fps*2,bytes:fs.statSync(file).size,sha256:sha(file)});}

}
if(kind==='video'&&displayCandidate){
 const {prepare}=require('./prepare-sywp.cjs'),{encode,read,display}=require('./sywp.cjs');
 const source=path.join(out,'test-pattern-30fps.mp4');
 for(const [name,width,height,fit,orientation,encoding] of [
  ['test-30fps-640x360-contain',640,360,'contain','auto','rgb565'],
  ['test-20fps-640x480-E6-stretch',640,480,'stretch','auto','rgb565'],
  ['test-30fps-360x640-portrait-only',360,640,'cover','portrait','rgb565']]){
  const fps=name.includes('20fps')?20:30,file=path.join(out,name+'.sywp');prepare(source,file,{title:name,width,height,fit,orientation,encoding,fps,frames:fps*2,gop:1});const manifest=read(file).manifest;
  if(manifest.width!==width||manifest.height!==height||manifest.display.fit!==fit||manifest.display.orientation!==orientation)throw Error('Display sample mismatch');
  samples.push({name:path.basename(file),encoding,fps,width,height,frames:manifest.frames,fit,orientation,bytes:fs.statSync(file).size,sha256:sha(file)});
 }
 for(const name of ['responsive','clock']){
  const file=path.join(out,'test-web-'+name+'.sywp');fs.writeFileSync(file,encode({format:'sywp',version:1,title:'BelleWall '+name,kind:'web',width:180,height:320,entry:'index.html',loop:true,pause:'resume',display:{...display,fit:'contain'}},fs.readFileSync(path.join(root,'prototype/content',name+'.html'))));read(file);
  samples.push({name:path.basename(file),encoding:'html',bytes:fs.statSync(file).size,sha256:sha(file)});
 }
}

const report={version,status:kind==='video'?'offline-built-awaiting-E7-and-603-video-test':'offline-built-awaiting-E7-device-test',installer:path.basename(installer),sha256:sha(installer),components:components.map(([name,uid])=>({name,uid,sha256:sha(path.join(out,name))})),samples,romProfileVerification:'passed; see rom-profile-verification.log',genericLayoutVerification:'unique structural match on three ROM images; see generic-layout-verification.log',layoutProfiles:['Nokia 603 RM-779 113.010.1506','Nokia E7-00 RM-626 111.040.1511'],genericStaticHoldout:'Nokia 603 RM-779 111.020.0310',qtMinimum:'compiled Qt 4.7.4',pageCount:'1-8 default black pages',videoPlayback:kind==='video'?'180x320 or 360x640; RGB565 request ceiling 30 fps; MP4V GetFrame path in 1.0.3 is experimental and device performance unmeasured':'10 fps playback gate',libraryDrives:kind==='video'?['C','E','F']:['C'],limitations:kind==='video'?['20/30 fps and cross-drive playback still require E7 and 603 acceptance tests.','A 30 fps request ceiling is not a measured LCD frame rate.']:['E7 profile and generic fallback have not been accepted by a real playback and restoration test.','A structural match is an admission check, not proof that every Belle firmware behaves correctly.']};
if(displayCandidate){report.status='offline-built-awaiting-device-rotation-language-and-performance-tests';report.videoPlayback='RGB565 only; MP4 wallpaper support temporarily disabled; request ceiling 30 fps; actual decoded/presented fps unmeasured';report.display={maximumDimension:2048,maximumPixels:1048576,fit:['cover','contain','stretch'],rotation:'pause, rebuild, resume; device validation pending',web:'responsive callback or authored-size fallback'};report.languages=['zh','en'];report.limitations=['E7 and 603 rotation, recovery and performance require device acceptance.','E6 screen geometry is covered offline; E6 firmware and device compatibility are unverified.','MP4 wallpapers are disabled after device frame extraction/cleanup hang; MP4 sources can be converted to RGB565 on PC.'];}
fs.writeFileSync(path.join(out,'validation.json'),JSON.stringify(report,null,2)+'\n');
const readme=kind==='video'
 ? '# BelleWall '+version+' 视频候选\n\n这是 E7 与 603 实机测试包。先在旧版停止壁纸并完成桌面恢复，然后只安装 `'+path.basename(installer)+'`。组合包内含三个同版本组件；导入的壁纸库不在安装包载荷中。\n\n目录中的 `test-20fps-360x640.sywp` 和 `test-30fps-360x640.sywp` 是各两秒的 RGB565 动态测试图；1.0.3 另有 `test-30fps-360x640-mp4.sywp` 压缩版测试图。壁纸管理导入 SYWP 时，选文件后再选 C、E 或 F 存储盘；列表显示盘符。旧版 C 盘库和选中记录继续可读。原始 SYWP 文件不会被移动或删除。RGB565 视频支持 180×320／360×640，按素材时间最高 30 fps 请求帧；MP4 压缩版使用系统逐帧解码接口，速度需实机测试。请在两台手机上比较两种格式的导入耗时、实际流畅度、暂停恢复、停止后的桌面恢复，以及 E／F 盘读取与删除。C 盘仍保存少量配置、日志和恢复记录。\n\nE7 请保持竖屏并把当前页面改为原生默认黑色背景；603 也需原生默认黑色背景。测试过程先选“60 秒检查”。失败时记下错误码并从应用内导出诊断。30 fps 是调度上限，不是实机已达到的帧率。离线校验与哈希见 `validation.json`。\n'
 : '# BelleWall 1.0.1 兼容性候选\n\n这是离线构建的实机测试包，尚未在 E7 播放验收。安装前在旧版中停止壁纸并确认桌面已恢复；升级时只安装 `BelleWall-1.0.1-compat-candidate.sisx`，它包含三个配套组件。不要单独重复安装目录内的组件 SISX。导入的壁纸库不在本包载荷中。\n\nE7 测试：保持竖屏；当前只读检查显示 E7 有一页，背景为 `Clouds.jpg`，请先把这一页改为原生默认黑色背景。候选包接受 1 至 8 页。打开 BelleWall，选择已导入的壁纸并启动。观察能否进入播放、桌面是否实际更新；停止后检查原生背景是否恢复。若报错，请记下错误码，在应用内运行诊断并保存日志。\n\n603 FP2 与 E7 使用各自已核验的精确布局；未知固件可通过带通配位的结构匹配推导背景偏移和虚表地址。Nokia 603 的旧固件 111.020.0310 作为第三份 ROM 样本离线唯一命中，但没有实机验收。结构不匹配或多重命中会拒绝，不能据此声称所有 Belle 固件已兼容。Qt 运行库最低要求为编译版本 4.7.4。证书与设备权限仍须满足系统安装条件。验证明细及哈希见 `validation.json`，静态证据见 `rom-profile-verification.log` 和 `generic-layout-verification.log`。\n';
fs.writeFileSync(path.join(out,'README.md'),displayCandidate?fs.readFileSync(path.join(root,'doc/'+(release?'RELEASE-':'CANDIDATE-')+version+'.md'),'utf8'):readme);
console.log('Candidate: '+installer);
