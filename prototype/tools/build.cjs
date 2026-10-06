require('./generate-ui-strings.cjs');
'use strict';
// Direct GCCE build; does not write into the SDK or depend on SBS/Perl drive mapping.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'), sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474';
const gcc=process.env.BELLE_GCCE||'C:/QtSDK/Symbian/tools/gcce4';
const out=path.join(root,'build/arm'),dist=path.join(root,'dist');fs.mkdirSync(out,{recursive:true});fs.mkdirSync(dist,{recursive:true});
const log=[];
function run(exe,args,cwd=out){log.push(JSON.stringify({exe,args}));const r=cp.spawnSync(exe,args,{cwd,env:{...process.env,PATH:sdk+'/epoc32/tools;'+process.env.PATH},encoding:'utf8',maxBuffer:16*1024*1024});log.push(r.stdout||'',r.stderr||'');fs.writeFileSync(path.join(out,'build.log'),log.join('\n'));if(r.error)throw r.error;if(r.status!==0){console.error(r.stdout,r.stderr);throw Error('Build failed: '+path.basename(exe));}if(r.stdout)console.log(r.stdout);}
const inc=sdk+'/epoc32/include',lib=sdk+'/epoc32/release/armv5/lib';
const flags=['-c','-g','-O2','-marm','-march=armv6','-mfloat-abi=softfp','-mfpu=vfp','-msoft-float','-fexceptions','-fno-unit-at-a-time','-fno-strict-aliasing','-fvisibility=hidden','-fvisibility-inlines-hidden','-Wall','-Wextra','-Wno-unused-parameter','-D__SUPPORT_CPP_EXCEPTIONS__','-D__LEAVE_EQUALS_THROW__','-DSYMBIAN_ENABLE_SPLIT_HEADERS','-D__SYMBIAN_STDCPP_SUPPORT__','-D__SYMBIAN32__','-D__EPOC32__','-D__GCCE__','-D__MARM__','-D__EABI__','-D__MARM_ARMV5__','-D__EXE__','-D_UNICODE','-DUNICODE','-DNDEBUG','-DQT_NO_DEBUG','-DQT_SHARED','-DQT_GUI_LIB','-DQT_CORE_LIB','-DQT_WEBKIT_LIB','-DQT_NETWORK_LIB','-include',inc+'/gcce/gcce.h'];
for(const p of [inc,inc+'/variant',inc+'/platform',inc+'/platform/graphics',inc+'/mw',inc+'/platform/mw',inc+'/platform/mw/alf',inc+'/stdapis',inc+'/stdapis/stlportv5',sdk+'/include',sdk+'/include/QtCore',sdk+'/include/QtGui',sdk+'/include/QtWebKit',sdk+'/include/QtNetwork',sdk+'/include/QtScript'])flags.push('-I'+p);
run(gcc+'/bin/arm-none-symbianelf-g++.exe',[...flags,path.join(root,'prototype/src/main.cpp'),'-o','main.o']);
const decoder=path.join(root,'prototype/vendor/h264bsd/src');
const decoderObjects=fs.readdirSync(decoder).filter(n=>n.endsWith('.c')).map(n=>{
 const object=n.replace(/\.c$/,'.o');run(gcc+'/bin/arm-none-symbianelf-gcc.exe',['-c','-std=gnu99','-O2','-marm','-march=armv6','-msoft-float','-D__SYMBIAN32__','-D__GCCE__','-D__EABI__','-D__MARM__','-include',inc+'/gcce/gcce.h','-DNDEBUG','-I'+inc+'/stdapis','-I'+inc,path.join(decoder,n),'-o',object]);return object;
});
const libs=['drtaeabi','dfpaeabi','scppnwdl','libstdcppv5','libstdcpp','hash','QtScript','QtWebKit','QtGui','QtCore','QtNetwork','euser','efsrv','ws32','gdi','fbscli','bitgdi','apgrfx','apparc','cone','eikcore','avkon','bafl','hal','centralrepository','mediaclientvideo','mediaclientaudio','alfdecoderserverclient','hwrmlightclient','libEGL','libGLESv1_CM','libc','libm','libdl','libpthread'];
run(gcc+'/bin/arm-none-symbianelf-ld.exe',['--target1-rel','--no-undefined','-nostdlib','--default-symver','-Ttext','0x8000','-Tdata','0x400000','--entry','_E32Startup','-u','_E32Startup','-o','bellewall.elf',sdk+'/epoc32/release/armv5/urel/eexe.lib','main.o',...decoderObjects,'--start-group',sdk+'/epoc32/release/armv5/urel/qtmain.lib',...libs.map(n=>lib+'/'+n+'.dso'),sdk+'/epoc32/release/armv5/urel/libcrt0.lib',sdk+'/epoc32/release/armv5/urel/usrt3_1.lib',gcc+'/arm-none-symbianelf/lib/libsupc++.a',gcc+'/lib/gcc/arm-none-symbianelf/4.4.1/libgcc.a','--end-group']);
for(const [name,uid] of [['bellewall','0xe7b31101'],['belleweb','0xe7b31130']])run(sdk+'/epoc32/tools/elf2e32.exe',['--elfinput=bellewall.elf','--output='+dist+'/'+name+'.exe','--uid1=0x1000007a','--uid2=0x100039ce','--uid3='+uid,'--sid='+uid,'--vid=0','--targettype=EXE','--capability=ReadUserData+WriteUserData+ReadDeviceData+WriteDeviceData+SwEvent+ProtServ+PowerMgmt','--heap=0x100000,0x2000000','--stack=0x20000','--linkas='+name+'.exe','--libpath='+lib,'--fpu=softvfp','--debuggable']);
for(const name of ['bellewall','bellewall_reg','belleweb_reg']){
 run(gcc+'/bin/arm-none-symbianelf-gcc.exe',['-E','-P','-x','c++','-I'+inc,path.join(root,'prototype/src',name+'.rss'),'-o',name+'.rpp']);
 run(sdk+'/epoc32/tools/rcomp.exe',['-u','-s'+name+'.rpp','-o'+path.join(dist,name+'.rsc'),'-h'+name+'.rsg']);
}
fs.copyFileSync(path.join(dist,'bellewall.rsc'),path.join(dist,'belleweb.rsc'));
const iconWork=fs.mkdtempSync(path.join(require('os').tmpdir(),'bellewall-icon-'));
try{
 fs.copyFileSync(path.join(root,'assets/branding/bellewall-icon.svg'),path.join(iconWork,'icon.svg'));
 run(sdk+'/epoc32/tools/mifconv.exe',['bellewall.mif','-T'+iconWork,'-c32,8','icon.svg'],iconWork);
 fs.copyFileSync(path.join(iconWork,'bellewall.mif'),path.join(dist,'bellewall.mif'));
}finally{
 const resolved=fs.realpathSync(iconWork),parent=fs.realpathSync(require('os').tmpdir());
 if(path.dirname(resolved)!==parent||!path.basename(resolved).startsWith('bellewall-icon-'))throw Error('Unsafe icon temporary directory');
 fs.rmSync(resolved,{recursive:true,force:true});
}
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const sources=fs.readdirSync(path.join(root,'prototype/src')).map(n=>'prototype/src/'+n).concat('prototype/tools/build.cjs','assets/branding/bellewall-icon.svg',fs.readdirSync(decoder).map(n=>'prototype/vendor/h264bsd/src/'+n));
fs.writeFileSync(path.join(out,'success.json'),JSON.stringify({sdk,gcc,sources:Object.fromEntries(sources.map(n=>[n,sha(path.join(root,n))])),exeSha256:sha(path.join(dist,'bellewall.exe')),workerSha256:sha(path.join(dist,'belleweb.exe')),resources:Object.fromEntries(['bellewall.rsc','bellewall_reg.rsc','belleweb.rsc','belleweb_reg.rsc','bellewall.mif'].map(n=>[n,sha(path.join(dist,n))])),level:'Compiled and linked only; no emulator or device execution'},null,2));
console.log('ARM executable: '+dist+'/bellewall.exe');










