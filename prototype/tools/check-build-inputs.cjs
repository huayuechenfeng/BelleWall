'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
function check(){
 const root=path.resolve(__dirname,'../..'),env=process.env;
 if(process.platform!=='win32')throw Error('The phone release build requires Windows and the Symbian SDK.');
 if(Number(process.versions.node.split('.')[0])<22)throw Error('Use Node.js 22 or later (browser QA needs built-in WebSocket).');
 const sdk=env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474';
 const gcc=env.BELLE_GCCE||'C:/QtSDK/Symbian/tools/gcce4';
 const requireFile=file=>{if(!fs.existsSync(file)||!fs.statSync(file).isFile())throw Error('Missing build input: '+file+'; see doc/BUILD.md');};
 for(const name of ['elf2e32','makesis','signsis','dumpsis','rcomp','mifconv'])requireFile(path.join(sdk,'epoc32/tools',name+'.exe'));
 for(const name of ['g++','gcc','ld'])requireFile(path.join(gcc,'bin','arm-none-symbianelf-'+name+'.exe'));
 requireFile(path.join(gcc,'lib/gcc/arm-none-symbianelf/4.4.1/libgcc.a'));
 requireFile(path.join(sdk,'include/QtCore/qglobal.h'));
 const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
 for(const row of fs.readFileSync(path.join(root,'prototype/baseline/SHA256SUMS.txt'),'utf8').trim().split(/\r?\n/)){
  const file=path.join(root,'prototype/baseline',row.slice(66));requireFile(file);if(sha(file)!==row.slice(0,64))throw Error('Public baseline hash mismatch: '+file);
 }
 const roms=require('./rom-inputs.cjs').load();
 const portable=path.resolve(env.BELLEWALL_PORTABLE_FFMPEG||path.join(root,'build/ffmpeg-portable/package'));
 requireFile(path.join(portable,'SOURCE.json'));
 const provenance=JSON.parse(fs.readFileSync(path.join(portable,'SOURCE.json'),'utf8'));
 if(sha(path.join(portable,'bin/ffmpeg.exe'))!==provenance.binarySha256||sha(path.join(portable,'source/ffmpeg-9.0.1.tar.xz'))!==provenance.sourceSha256)throw Error('Portable FFmpeg provenance mismatch');
 const probes=[
  [path.join(gcc,'bin/arm-none-symbianelf-g++.exe'),['--version']],
  [env.BELLEWALL_HOST_CXX||'C:/msys64/usr/bin/g++.exe',['--version']],
  [env.OPENSSL||'C:/Program Files/Git/mingw64/bin/openssl.exe',['version']],
  [require('./ffmpeg.cjs').resolveFFmpeg(),['-version']],
  [env.BELLEWALL_TEST_FFMPEG||'ffmpeg',['-version']]
 ];
 const tools=probes.map(([exe,args])=>{const r=cp.spawnSync(exe,args,{encoding:'utf8',windowsHide:true});if(r.error||r.status)throw Error('Cannot run '+exe+': '+(r.error||r.stderr));return {exe,version:r.stdout.split(/\r?\n/)[0]};});
 requireFile(env.BELLE_BROWSER||'C:/Program Files/Google/Chrome/Application/chrome.exe');
 const result={status:'passed',node:process.version,sdk,gcc,tools,roms,portable};console.log(JSON.stringify(result,null,2));return result;
}
module.exports={check};
if(require.main===module)check();
