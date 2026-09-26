'use strict';
// Longrun integration: new renderer plus legacy recovery files, paired native/UI.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||path.join(root,'dist/maintenance-0.4.3-20260926'));
if(fs.existsSync(out))throw Error('Checkpoint exists; select a new destination');
const assets=path.join(root,'build/product-assets'),renderer=path.join(root,'dist/daily-candidate-20260920-r3');
if(!fs.existsSync(path.join(assets,'PROVENANCE.md')))throw Error('Run prepare-product-assets.cjs first');
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
function walk(p){return fs.readdirSync(p,{withFileTypes:true}).flatMap(e=>{if(e.isSymbolicLink())throw Error('Source symlink rejected');return e.isDirectory()?walk(path.join(p,e.name)):[path.join(p,e.name)];});}
function copy(from,to){fs.mkdirSync(path.dirname(to),{recursive:true});fs.copyFileSync(from,to);}
function tree(from,to){for(const f of walk(from))copy(f,path.join(to,path.relative(from,f)));}
function verify(dir){for(const l of fs.readFileSync(path.join(dir,'SHA256SUMS.txt'),'utf8').trim().split(/\r?\n/)){if(sha(path.join(dir,l.slice(66)))!==l.slice(0,64))throw Error('Checkpoint changed: '+dir);}}
verify(path.join(root,'dist/device-candidate-20260920-r10'));verify(path.join(root,'dist/product-preview-0.3.1-20260920-r2'));verify(renderer);verify(path.join(root,'dist/longrun-candidate-0.4.0-20260925-r9'));
fs.mkdirSync(out,{recursive:true});const evidence=path.join(out,'evidence');fs.mkdirSync(evidence);
function run(tool,args=[],label=path.basename(tool)){const r=cp.spawnSync(process.execPath,[tool,...args],{cwd:root,encoding:'utf8',maxBuffer:32*1024*1024,windowsHide:true,env:{...process.env,BELLEWALL_DEMO_MP4:path.join(assets,'demo.mp4')}});fs.writeFileSync(path.join(evidence,label+'.log'),(r.stdout||'')+(r.stderr||''));if(r.error||r.status)throw Error(label+' failed: '+(r.error||r.status));console.log(label+' passed');}
run('prototype/tools/check-docs.cjs',[],'documentation-check');
run('--test',['prototype/tests/mpkg.test.cjs','prototype/tests/sywp.test.cjs','prototype/tests/device-session.test.cjs','prototype/tests/product-flow.test.cjs','prototype/tests/longrun.test.cjs','prototype/tests/consumer-watch.test.cjs','prototype/tests/renderer-identity.test.cjs','prototype/tests/preparation.test.cjs','prototype/tests/preparation-transaction.test.cjs','prototype/tests/library-delete.test.cjs'],'offline-tests');
const acceptedRenderer=path.join(root,'dist/renderer-integration-0.4.1-20260926-r6/renderer');verify(acceptedRenderer);
tree(acceptedRenderer,path.join(out,'renderer'));
fs.writeFileSync(path.join(evidence,'reuse-renderer.json'),JSON.stringify({checkpoint:'renderer-integration-0.4.1-20260926-r6',dllSha256:sha(path.join(acceptedRenderer,'bellerenderlongrun.dll')),resourceSha256:sha(path.join(acceptedRenderer,'bellerenderlongrun.rsc'))},null,2));
console.log('accepted r6 renderer retained byte-for-byte');
run('prototype/tools/build-render-probe.cjs',['--host-only'],'build-render-helper');
run('prototype/tools/package-session-helper.cjs',[path.join(out,'renderer')]);
for(const tool of ['make-background-profile.cjs','build-native-wallpaper.cjs','build.cjs','package.cjs','check-delivery.cjs'])run('prototype/tools/'+tool);
run('prototype/tools/verify-product-packages.cjs',['dist',path.join(evidence,'paired-packages.json')]);
run('--test',['prototype/tests/package-verifier.test.cjs'],'package-verifier-tests');
const nativeApp=['bellerenderlongrun.dll','bellerenderlongrun.rsc','bellerenderhost.exe','bellerender-selfsigned.sisx','bellepaper.exe','bellepaper-selfsigned.sisx','bellewall.exe','bellewall.rsc','bellewall_reg.rsc','belleweb.exe','belleweb.rsc','belleweb_reg.rsc','bellewall-selfsigned.sisx'];
for(const name of nativeApp)copy(path.join(root,'dist',name),path.join(out,name));
for(const name of ['config.ini','animation.html','sample.mp4','THIRD-PARTY.txt'])copy(path.join(root,'dist',name),path.join(out,'package-inputs',name));
const retained=['bellerendercandidate.dll','bellerendercandidate.rsc'];
for(const name of retained)copy(path.join(renderer,name),path.join(out,name));
for(const name of ['video.sywp','web.sywp','corrupt.sywp','unsupported.sywp','PROVENANCE.md'])copy(path.join(assets,name),path.join(out,'examples',name));
tree(path.join(root,'prototype'),path.join(out,'source/prototype'));
for(const rel of ['research/sources/homescreen-s3/idlehomescreen/sapiwrapper/hspswrapper/inc','research/sources/homescreen-s3/idlehomescreen/inc'])tree(path.join(root,rel),path.join(out,'source',rel));
for(const name of ['desktop.png','mobile.png','result.json'])copy(path.join(root,'build/product-browser-qa',name),path.join(out,'evidence/browser',name));
copy(path.join(renderer,'validation.json'),path.join(out,'provenance/renderer/validation.json'));
tree(path.join(renderer,'source'),path.join(out,'provenance/renderer/source'));
const fileNames=[...nativeApp,...retained,...['video','web','corrupt','unsupported'].map(n=>'examples/'+n+'.sywp')];
fs.writeFileSync(path.join(out,'validation.json'),JSON.stringify({version:'0.4.3-maintenance-candidate',status:'offline-built-device-longrun-pending',target:'Nokia 603 RM-779 / 113.010.1506 / Qt 4.8.1 / four default black pages',acceptedBaseline:'../device-candidate-20260920-r10',renderer:{dllRevision:'accepted-r6-longrun-v2',dllUnchanged:true,legacyR3Retained:true,helperVersion:'0.4.3',sessionVersion:2,configurationUid:'0x70031129'},runtime:'60/600 seconds or explicit continuous mode; no autostart; longrun device validation pending',files:fileNames.map(name=>({name,sha256:sha(path.join(out,name))})),scope:'Longrun renderer/session-v2 integration candidate only. Install helper -> native -> Qt together. Application preparation and uninstall cleanup with durable retry journals and local diagnostics. This newly built checkpoint requires change-scoped acceptance; prior device evidence remains external and does not automatically certify changed binaries. Not a public release.'},null,2));
fs.writeFileSync(path.join(out,'README.md'),"# BelleWall 0.4.3 候选构建快照\n\n这是构建时冻结的离线检查点，不表示已经安装或通过实机验收。三个顶层 SIS 按 helper → native → Qt 配套安装；升级前停止并完成恢复。已测 r6 渲染 DLL／资源和 r3 旧恢复文件保持原样。\n\n主界面直接提供壁纸管理、恢复桌面、运行诊断、检查并准备组件、卸载准备和返回桌面，没有更多菜单。壁纸管理支持导入、预览、删除和启动；运行期间不可用。维护清理精确核验新旧两代身份并移除注册，保留壁纸库。\n\n见 [使用说明](source/doc/development/MAINTENANCE-0.4.3.md)、[构建时状态](source/doc/development/CURRENT-STATE.md)。已有实机结果属于其各自检查点；新构建按变更范围另验，后续结果记在项目 research/evidence，不回写本快照。当前用户要求仅完成视频／网页各半小时测试，取消更长测试，不宣称数小时稳定性或续航已验证。\n");
const files=walk(out).sort();fs.writeFileSync(path.join(out,'SHA256SUMS.txt'),files.map(f=>sha(f)+'  '+path.relative(out,f).replaceAll('\\','/')).join('\n')+'\n');verify(out);console.log('Product preview checkpoint: '+out);
