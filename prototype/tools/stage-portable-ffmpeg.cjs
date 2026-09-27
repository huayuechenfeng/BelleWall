'use strict';
// Stage the native Windows converter plus matching source and license material.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),base=path.join(root,'build/ffmpeg-portable');
const source=path.join(base,'ffmpeg-9.0.1'),out=path.resolve(process.argv[2]||path.join(base,'package'));
if(fs.existsSync(out))throw Error('Choose a clean staging directory; preserve previous release artifacts');
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const sourceSha='cf38e0e28c7e5605942c4a77755349b0145804a397af37eb1fb4c77cb237f635';
if(sha(path.join(base,'ffmpeg-9.0.1.tar.xz'))!==sourceSha)throw Error('FFmpeg source hash mismatch');
function copy(from,to){const dst=path.join(out,to);fs.mkdirSync(path.dirname(dst),{recursive:true});fs.copyFileSync(from,dst);}
copy(path.join(source,'bellewall-install/bin/ffmpeg.exe'),'bin/ffmpeg.exe');
copy(path.join(base,'ffmpeg-9.0.1.tar.xz'),'source/ffmpeg-9.0.1.tar.xz');
copy(path.join(root,'prototype/tools/build-portable-ffmpeg.sh'),'source/build-portable-ffmpeg.sh');
for(const file of ['COPYING.LGPLv2.1','LICENSE.md'])copy(path.join(source,file),file);
const msys=process.env.BELLEWALL_MSYS_ROOT||'C:/msys64';
for(const component of ['gcc-libs','crt','headers','libwinpthread']){
 const dir=path.join(msys,'ucrt64/share/licenses',component);
 for(const file of fs.readdirSync(dir))copy(path.join(dir,file),'licenses/'+component+'/'+file);
}
const exe=path.join(out,'bin/ffmpeg.exe');
const version=cp.execFileSync(exe,['-version'],{encoding:'utf8',windowsHide:true});
if(!version.startsWith('ffmpeg version 9.0.1')||version.includes('--enable-gpl')||version.includes('--enable-nonfree'))throw Error('Unexpected converter configuration');
const imports=cp.execFileSync(path.join(msys,'ucrt64/bin/objdump.exe'),['-p','ffmpeg.exe'],{cwd:path.dirname(exe),encoding:'utf8',maxBuffer:8*1024*1024}).split('\n').filter(line=>line.includes('DLL Name:')).map(line=>line.trim());
if(imports.some(line=>/msys-|libgcc|libwinpthread|libstdc\+\+/i.test(line)))throw Error('Non-system runtime DLL dependency');
fs.writeFileSync(path.join(out,'DLL-IMPORTS.txt'),imports.join('\n')+'\n');
fs.writeFileSync(path.join(out,'BUILD.txt'),version);
fs.writeFileSync(path.join(out,'SOURCE.json'),JSON.stringify({version:'9.0.1',platform:'Windows x64',source:'https://ffmpeg.org/releases/ffmpeg-9.0.1.tar.xz',sourceSha256:sourceSha,binarySha256:sha(exe),modifiedSource:false,license:'LGPL-2.1-or-later',compiler:cp.execFileSync(path.join(msys,'ucrt64/bin/gcc.exe'),['--version'],{encoding:'utf8'}).split('\n')[0]},null,2)+'\n');
fs.writeFileSync(path.join(out,'README.md'),`# Bundled FFmpeg for BelleWall\n\nFFmpeg 9.0.1, Copyright (c) the FFmpeg developers, built from unmodified official source. Windows x64, UCRT (Windows 10/11). This executable runs as a separate process and is licensed under LGPL-2.1-or-later, not BelleWall's MIT license. See COPYING.LGPLv2.1 and LICENSE.md.\n\nThe complete matching FFmpeg source archive and build script are included under source/. Extract the archive, install MSYS2 UCRT64 gcc and make, then run source/build-portable-ffmpeg.sh from the extracted source directory. BUILD.txt records the configuration; SOURCE.json records hashes and compiler. Compiler/runtime notices are in licenses/. Native Windows system libraries are not bundled.\n\nOnly local video decoding, image conversion, raw frame output and MPEG-4 Part 2 MP4 encoding are enabled. No external codec libraries, network access, ffplay or ffprobe are included. Unsupported inputs can be converted first, or use a replacement FFmpeg via FFMPEG_BIN. No FFmpeg source changes were made.\n\nUpstream: https://ffmpeg.org/\nSource: https://ffmpeg.org/releases/ffmpeg-9.0.1.tar.xz\nToolchain: https://www.msys2.org/ and https://www.mingw-w64.org/\n`);
console.log(out);
