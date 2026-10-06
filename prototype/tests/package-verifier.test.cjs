'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),path=require('path'),os=require('os'),cp=require('child_process');
const {verify}=require('../tools/verify-product-packages.cjs');
const root=path.resolve(__dirname,'../..');
const currentVersion=(process.env.BELLEWALL_PRODUCT_VERSION||'1.0.0').replaceAll('.',',');
const flat=path.join(root,'dist');
const snapshots={'1,1,1':path.join(root,'build/checkpoints/BelleWall-1.1.1-redraw-handshake-v1/combined-verify'),'1,1,0':path.join(root,'build/checkpoints/BelleWall-1.1.0-display-language-candidate-v1/combined-verify'),'1,0,3':path.join(root,'build/checkpoints/BelleWall-1.0.3-dual-video-candidate-v2/combined-verify'),'1,0,0':path.join(root,'build/release-1.0.0/combined-verify'),'1,0,1':path.join(root,'build/checkpoints/BelleWall-1.0.1-compat-candidate-v7/combined-verify'),'1,0,2':path.join(root,'build/checkpoints/BelleWall-1.0.2-video-candidate-v2/combined-verify')};
const dist=fs.existsSync(path.join(flat,'bellepaper.exe'))?flat:snapshots[currentVersion];
if(!dist)throw Error('No package verification snapshot for '+currentVersion);
const names=['bellerender-selfsigned.sisx','bellepaper-selfsigned.sisx','bellewall-selfsigned.sisx','bellerendercandidate.dll','bellerendercandidate.rsc','bellerenderhost.exe','bellerenderlongrun.dll','bellerenderlongrun.rsc','bellepaper.exe','bellewall.exe','bellewall.rsc','bellewall_reg.rsc','config.ini','animation.html','sample.mp4','THIRD-PARTY.txt','belleweb.exe','belleweb.rsc','belleweb_reg.rsc'];
function fixture(fn){const dir=fs.mkdtempSync(path.join(os.tmpdir(),'bellewall-package-fixture-'));try{for(const n of names){const direct=path.join(dist,n),source=fs.existsSync(direct)?direct:path.join(dist,'package-inputs',n);fs.copyFileSync(source,path.join(dir,n));}return fn(dir);}finally{const target=fs.realpathSync(dir);assert.equal(path.dirname(target),fs.realpathSync(os.tmpdir()));assert.match(path.basename(target),/^bellewall-package-fixture-/);fs.rmSync(target,{recursive:true,force:true});}}
function repackNative(dir,{uid='0xE7B31103',destination='C:\\sys\\bin\\bellepaper.exe',dependency=true,version=currentVersion,dependencyVersion=currentVersion}={}){
 const pkg=path.join(dir,'native-test.pkg');fs.writeFileSync(pkg,'&EN\n#{"Native fixture"},('+uid+'),'+version+'\n%{"Test"}\n:"Test"\n'+(dependency?'(0xE7B31106),'+dependencyVersion+',{"Helper"}\n':'')+'"'+path.join(dir,'bellepaper.exe').replaceAll('\\','/')+'"-"'+destination+'"\n');
 const built=path.join(dir,'native-test.sis');const r=cp.spawnSync(path.join(process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474','epoc32/tools/makesis.exe'),[pkg,built],{encoding:'utf8',windowsHide:true});if(r.error||r.status)throw Error(r.error||r.stdout+r.stderr);fs.copyFileSync(built,path.join(dir,'bellepaper-selfsigned.sisx'));
}
test('actual paired SIS extraction verifies all payloads, application identities and capabilities',()=>{const report=verify(dist);assert.equal(report.packages.length,3);assert.equal(report.payloadCount,16);assert.equal(report.packages.flatMap(p=>p.payloads).filter(p=>p.uid3).length,6);});
test('a modified expected native executable is rejected against the real signed SIS',()=>fixture(dir=>{const p=path.join(dir,'bellepaper.exe'),b=fs.readFileSync(p);b[b.length-1]^=1;fs.writeFileSync(p,b);assert.throws(()=>verify(dir),/payload mismatch bellepaper\.exe/);}));
test('a package copied under another component name is rejected',()=>fixture(dir=>{fs.copyFileSync(path.join(dir,'bellewall-selfsigned.sisx'),path.join(dir,'bellerender-selfsigned.sisx'));assert.throws(()=>verify(dir),/unexpected payload count/);}));
test('unchanged payload installed to a different destination is rejected',()=>fixture(dir=>{repackNative(dir,{destination:'C:\\data\\BelleWall\\bellepaper.exe'});assert.throws(()=>verify(dir),/install destination\/options mismatch/);}));
test('native package without its helper dependency is rejected',()=>fixture(dir=>{repackNative(dir,{dependency:false});assert.throws(()=>verify(dir),/package dependency mismatch/);}));
test('package UID cannot differ while all payload bytes remain identical',()=>fixture(dir=>{repackNative(dir,{uid:'0xE7B31109'});assert.throws(()=>verify(dir),/package identity\/version mismatch/);}));

test('old native version is rejected despite matching new payloads',()=>fixture(dir=>{repackNative(dir,{version:'0,4,2'});assert.throws(()=>verify(dir),/package identity\/version mismatch/);}));
test('old helper minimum version is rejected for the new native package',()=>fixture(dir=>{repackNative(dir,{dependencyVersion:'0,4,2'});assert.throws(()=>verify(dir),/package dependency mismatch/);}));
