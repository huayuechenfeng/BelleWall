'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474',gcc=process.env.BELLE_GCCE||'C:/QtSDK/Symbian/tools/gcce4';
const version=process.env.BELLEWALL_PRODUCT_VERSION||'1.0.0';
if(!/^\d+\.\d+\.\d+$/.test(version))throw Error('Invalid product version');
const sisVersion=version.replaceAll('.',',');
require('./ensure-signing.cjs')(root);
fs.mkdirSync(path.join(root,'dist'),{recursive:true});
const out=path.join(root,'build/native-wallpaper'),dist=path.join(root,'dist'),inc=sdk+'/epoc32/include',lib=sdk+'/epoc32/release/armv5/lib';fs.mkdirSync(out,{recursive:true});
function run(exe,args){const r=cp.spawnSync(exe,args,{cwd:out,encoding:'utf8'});fs.appendFileSync(path.join(out,'build.log'),JSON.stringify({exe,args})+'\n'+r.stdout+r.stderr);if(r.error||r.status)throw Error(r.error||r.stdout+r.stderr);return r.stdout;}
run(gcc+'/bin/arm-none-symbianelf-g++.exe',['-I'+path.join(root,'prototype/vendor/symbian-homescreen/hspswrapper/inc'),'-fpermissive','-c','-O2','-g','-marm','-march=armv6','-mfloat-abi=softfp','-mfpu=vfp','-msoft-float','-fexceptions','-fno-unit-at-a-time','-fno-strict-aliasing','-D__SYMBIAN32__','-D__EPOC32__','-D__GCCE__','-D__MARM__','-D__EABI__','-D__MARM_ARMV5__','-D__EXE__','-D_UNICODE','-D__SUPPORT_CPP_EXCEPTIONS__','-D__LEAVE_EQUALS_THROW__','-DSYMBIAN_ENABLE_SPLIT_HEADERS','-include',inc+'/gcce/gcce.h','-I'+inc,'-I'+inc+'/platform','-I'+inc+'/mw','-I'+inc+'/platform/mw',path.join(root,'prototype/src/nativewallpaper.cpp'),'-o','probe.o']);
run(gcc+'/bin/arm-none-symbianelf-ld.exe',['--target1-rel','--no-undefined','-nostdlib','--default-symver','-Ttext','0x8000','-Tdata','0x400000','--entry','_E32Startup','-u','_E32Startup','-o','probe.elf',sdk+'/epoc32/release/armv5/urel/eexe.lib','probe.o','--start-group',...['hsccapiclient','hscontentinfo','hspswrapper','liwservicehandler','estor','ecom','hal','euser','efsrv','ws32','mediaclientvideo','mediaclientaudio','fbscli','bitgdi','gdi','apgrfx','centralrepository','aknswallpaperutils','aknskinsrv','hwrmlightclient','drtaeabi','dfpaeabi','scppnwdl'].map(n=>lib+'/'+n+'.dso'),sdk+'/epoc32/release/armv5/urel/usrt3_1.lib',gcc+'/arm-none-symbianelf/lib/libsupc++.a',gcc+'/lib/gcc/arm-none-symbianelf/4.4.1/libgcc.a','--end-group']);
run(sdk+'/epoc32/tools/elf2e32.exe',['--elfinput=probe.elf','--output='+dist+'/bellepaper.exe','--uid1=0x1000007a','--uid2=0','--uid3=0xe7b31103','--sid=0xe7b31103','--vid=0','--targettype=EXE','--capability=ReadUserData+WriteUserData+ReadDeviceData+WriteDeviceData+SwEvent+PowerMgmt','--heap=0x10000,0x800000','--stack=0x10000','--linkas=bellepaper.exe','--libpath='+lib,'--fpu=softvfp']);
fs.writeFileSync(path.join(out,'probe.pkg'),'&EN\n#{"BelleWall Native"},(0xE7B31103),'+sisVersion+'\n(0xE7B31106),'+sisVersion+',{"BelleWall Renderer Helper"}\n%{"BelleWall Research"}\n:"BelleWall Research"\n"'+dist.replaceAll('\\','/')+'/bellepaper.exe"-"C:\\sys\\bin\\bellepaper.exe"\n');
run(sdk+'/epoc32/tools/makesis.exe',['probe.pkg','probe.sis']);
run(sdk+'/epoc32/tools/signsis.exe',['-s','probe.sis',path.join(dist,'bellepaper-selfsigned.sisx'),path.join(root,'build/signing/prototype.cer'),path.join(root,'build/signing/prototype.key')]);
const report={purpose:'Bounded native wallpaper experiment with independent recovery guardian',capability:'ReadUserData+WriteUserData+ReadDeviceData+WriteDeviceData+SwEvent+PowerMgmt',exeSha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(dist,'bellepaper.exe'))).digest('hex')};
fs.writeFileSync(path.join(root,'research/evidence/device/native-wallpaper-build.json'),JSON.stringify(report,null,2));console.log(report);



