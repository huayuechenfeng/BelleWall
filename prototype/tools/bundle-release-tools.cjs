'use strict';
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||'build/release-1.0.0/tools');
if(fs.existsSync(out))throw Error('Choose a new tools directory');
const ffmpeg=path.resolve(process.env.BELLEWALL_PORTABLE_FFMPEG||path.join(root,'build/ffmpeg-portable/package'));
if(!fs.existsSync(path.join(ffmpeg,'bin/ffmpeg.exe')))throw Error('Build and stage portable FFmpeg first; see doc/BUILD.md');
const provenance=JSON.parse(fs.readFileSync(path.join(ffmpeg,'SOURCE.json'),'utf8'));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
if(hash(path.join(ffmpeg,'bin/ffmpeg.exe'))!==provenance.binarySha256||hash(path.join(ffmpeg,'source/ffmpeg-9.0.1.tar.xz'))!==provenance.sourceSha256)throw Error('Bundled FFmpeg provenance hash mismatch');
function copy(from,to=from){const dst=path.join(out,to);fs.mkdirSync(path.dirname(dst),{recursive:true});fs.copyFileSync(path.join(root,from),dst);}
for(const n of ['README.md','LICENSE'])copy(n);
for(const n of fs.readdirSync(path.join(root,'doc')).filter(n=>n.endsWith('.md')))copy('doc/'+n);
for(const n of fs.readdirSync(path.join(root,'LICENSES')))copy('LICENSES/'+n);
for(const n of ['sywp-webui.cjs','sywp-webui.html','sywp.cjs','mp4-profile.cjs','prepare-sywp.cjs','ffmpeg.cjs','mpkg.cjs'])copy('prototype/tools/'+n);
for(const n of ['clock.html','starter.html','responsive.html'])copy('prototype/content/'+n);
for(const kind of ['video','web'])copy('build/product-assets/'+kind+'.sywp','examples/BelleWall-'+kind+'.sywp');
copy('build/product-assets/demo.mp4','examples/demo.mp4');copy('build/product-assets/PROVENANCE.md','examples/PROVENANCE.md');
copy('build/product-assets/preview.mpkg','examples/demo-video.mpkg');
fs.writeFileSync(path.join(out,'Start-BelleWall.cmd'),'@echo off\r\nnode "%~dp0prototype\\tools\\sywp-webui.cjs"\r\nif errorlevel 1 pause\r\n');
if(process.env.BELLEWALL_PRODUCT_VERSION==='1.1.0')fs.writeFileSync(path.join(out,'README.md'),`# BelleWall 1.1.0 制作工具测试版 / Workshop test candidate

完整解压，保留 ffmpeg 文件夹，安装 Node.js 后双击 Start-BelleWall.cmd。文件在本机转换。右上角可切换中文／English，支持 RGB565 与 MP4V 两种 SYWP，可选 MP4 关键帧间隔。

横屏、640×480 等画布和 MP4 请搭配 1.1.0 手机测试候选。手机安装器在单独的 injector-test.zip，未包含在此 PC 工具包中。测试说明见 [候选验收清单](doc/CANDIDATE-1.1.0.md)。prototype/content/responsive.html 是响应屏幕尺寸的网页示例，examples 中保留既有入门素材。

Extract the complete archive, keep the ffmpeg folder, install Node.js, then run Start-BelleWall.cmd. Processing is local. Choose Chinese or English at the top right. Export either RGB565 or MP4V SYWP, with a configurable MP4 keyframe interval.

Use the matching 1.1.0 phone candidate for rotation, non-standard canvases and MP4. The phone installer is in the separate injector-test.zip. See [device test instructions](doc/CANDIDATE-1.1.0.md). This is a test build; hardware acceleration and sustained frame rates are unverified.
`);
const walk=p=>fs.readdirSync(p,{withFileTypes:true}).flatMap(e=>e.isDirectory()?walk(path.join(p,e.name)):[path.join(p,e.name)]);
for(const file of walk(ffmpeg))copy(path.relative(root,file),path.join('ffmpeg',path.relative(ffmpeg,file)));
const files=walk(out).sort();fs.writeFileSync(path.join(out,'SHA256SUMS.txt'),files.map(f=>crypto.createHash('sha256').update(fs.readFileSync(f)).digest('hex')+'  '+path.relative(out,f).replaceAll('\\','/')).join('\n')+'\n');
console.log('Tools bundle: '+out+' ('+files.length+' files)');
