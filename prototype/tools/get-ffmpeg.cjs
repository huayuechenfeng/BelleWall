'use strict';
// Download is task-local; never changes PATH or installs a global package.
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
const {Readable}=require('stream'),{pipeline}=require('stream/promises');
const url='https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-9.0.1-essentials_build.zip';
const expected='fec81ae03971d9dd4be3ebe02e263bd2ec1d789483f931bdba5f5715e65da2e9';
(async()=>{
 const root=path.resolve(__dirname,'../..'),build=path.join(root,'build'),archive=path.join(build,'ffmpeg-essentials.zip');fs.mkdirSync(build,{recursive:true});
 if(!fs.existsSync(archive)){const r=await fetch(url);if(!r.ok)throw Error('Download HTTP '+r.status);await pipeline(Readable.fromWeb(r.body),fs.createWriteStream(archive,{flags:'wx'}));}
 const actual=crypto.createHash('sha256').update(fs.readFileSync(archive)).digest('hex');if(actual!==expected)throw Error('FFmpeg archive hash mismatch; do not execute');
 const r=cp.spawnSync('tar.exe',['-xf',archive,'-C',build,'ffmpeg-9.0.1-essentials_build/bin/ffmpeg.exe','ffmpeg-9.0.1-essentials_build/bin/ffprobe.exe','ffmpeg-9.0.1-essentials_build/LICENSE'],{encoding:'utf8'});if(r.error||r.status)throw Error(r.error||r.stderr);
 console.log('Verified FFmpeg 9.0.1 extracted under build/');
})().catch(e=>{console.error(e.message);process.exitCode=1;});
