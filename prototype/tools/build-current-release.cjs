'use strict';
// Clean-checkout 1.3 release build (local signing changes binary hashes); publishing and phone installation are separate.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),version='1.3.0';
require('./check-build-inputs.cjs').check();
if(process.argv.includes('--check-inputs'))process.exit(0);
const snapshot=path.join(root,'build/checkpoints/BelleWall-'+version+'-release-v1');
const output=path.join(root,'dist/release',version),logs=path.join(root,'build/release-checkpoints/BelleWall-'+version);
if([snapshot,output,logs].some(p=>fs.existsSync(p)))throw Error('Release output exists; preserve the existing release and use a fresh checkout');
fs.mkdirSync(logs,{recursive:true});
const env={...process.env,BELLEWALL_PRODUCT_VERSION:version,BELLEWALL_CANDIDATE_VERSION:version,BELLEWALL_CHANNEL:'release',BELLEWALL_DEMO_MP4:path.join(root,'build/product-assets/demo.mp4')};
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
function run(script,args=[],label=path.basename(script)){
 const r=cp.spawnSync(process.execPath,[script,...args],{cwd:root,env,encoding:'utf8',windowsHide:true,maxBuffer:32*1024*1024});
 fs.writeFileSync(path.join(logs,label+'.log'),(r.stdout||'')+(r.stderr||''));
 if(r.error||r.status)throw Error(label+': '+(r.error||r.stderr||r.stdout));console.log(label+' passed');
}
run('prototype/tools/check-docs.cjs');
run('prototype/tools/prepare-product-assets.cjs');
run('prototype/tools/build-video-candidate.cjs',[snapshot],'phone-build');
run('--test',fs.readdirSync(path.join(root,'prototype/tests')).filter(n=>n.endsWith('.test.cjs')).map(n=>'prototype/tests/'+n),'offline-tests');
run('prototype/tools/check-webui-browser.cjs');
const tools=path.join(snapshot,'tools');run('prototype/tools/bundle-release-tools.cjs',[tools]);
fs.mkdirSync(output,{recursive:true});
const validation=JSON.parse(fs.readFileSync(path.join(snapshot,'validation.json'),'utf8'));
if(validation.version!==version||validation.installer!=='BelleWall-'+version+'.sisx'||sha(path.join(snapshot,validation.installer))!==validation.sha256)throw Error('Installer version or hash mismatch');
fs.copyFileSync(path.join(snapshot,validation.installer),path.join(output,validation.installer));
run('prototype/tools/package-release-tools.cjs',[tools,path.join(output,'BelleWall-'+version+'-tools.zip')]);
fs.copyFileSync(path.join(root,'doc/RELEASE-'+version+'.md'),path.join(output,'RELEASE-NOTES.md'));
const report={...validation,channel:'release',status:'offline-verified-release',display:{...validation.display,rotation:'E7 rotation previously user-tested; published 1.3.0 confirmed working on E7 on 2026-10-06'},deviceEvidence:'User confirmed published 1.3.0 working on E7 on 2026-10-06; rebuilt installers require their own acceptance.',limitations:['This newly rebuilt installer has offline verification only; E7 acceptance applies to the published 1.3.0 installer.','603 regression and E6 hardware behavior remain unverified.','Requested 30 fps is not a measured sustained frame rate; improved import time is not measured.','Compressed MP4 wallpapers are disabled; MP4 input can still be converted to RGB565.'],offlineChecks:['ROM profile and generic layout checks','ARM build and link','MIF icon compiled and included in verified phone payload','signed SIS component identity/version/dependency/destination/payload checks','complete offline test suite','browser bilingual and conversion QA','ZIP root layout and PC extracted payload hashes'],icon:{source:'assets/branding/bellewall-icon.svg',sha256:sha(path.join(root,'assets/branding/bellewall-icon.svg'))}};
fs.writeFileSync(path.join(output,'validation.json'),JSON.stringify(report,null,2)+'\n');
fs.writeFileSync(path.join(snapshot,'validation.json'),JSON.stringify(report,null,2)+'\n');
run('prototype/tools/package-video-test.cjs',[snapshot,path.join(output,'BelleWall-'+version+'-injector.zip')]);

const files=fs.readdirSync(output).sort();fs.writeFileSync(path.join(output,'SHA256SUMS.txt'),files.map(n=>sha(path.join(output,n))+'  '+n).join('\n')+'\n');
fs.writeFileSync(path.join(output,'CHANNEL.json'),JSON.stringify({channel:'release',version,artifacts:fs.readdirSync(output).sort().map(name=>({name,sha256:sha(path.join(output,name))}))},null,2)+'\n');
run('prototype/tools/verify-deliveries.cjs');
// Keep generated compiler/package output out of dist's public channel roots.
const flat=path.join(root,'dist'),working=path.join(snapshot,'working-dist');fs.mkdirSync(working);
for(const name of fs.readdirSync(flat)){const source=path.join(flat,name);if(fs.statSync(source).isFile()&&name!=='README.md')fs.renameSync(source,path.join(working,name));}
console.log('Release ready: '+output);
