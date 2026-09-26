'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),os=require('os'),path=require('path');
const {resolveFFmpeg}=require('../tools/ffmpeg.cjs');
test('FFmpeg discovery is portable and missing dependencies are actionable',()=>{
 const root=fs.mkdtempSync(path.join(os.tmpdir(),'bellewall-ffmpeg-'));
 try{
  const resolve=env=>resolveFFmpeg({root,env,platform:'win32'});
  assert.throws(()=>resolve({}),/未找到 FFmpeg/);
  const dir=path.join(root,'external tools'),exe=path.join(dir,'ffmpeg.exe');fs.mkdirSync(dir);fs.writeFileSync(exe,'');
  assert.equal(resolve({FFMPEG_BIN:dir}),exe);assert.equal(resolve({FFMPEG_BIN:exe}),exe);
  assert.equal(resolve({Path:dir}),exe);
  assert.throws(()=>resolve({FFMPEG_BIN:path.join(root,'missing'),PATH:dir}),/FFMPEG_BIN/);
  const portable=path.join(root,'ffmpeg/bin/ffmpeg.exe');fs.mkdirSync(path.dirname(portable),{recursive:true});fs.copyFileSync(exe,portable);
  assert.equal(resolve({}),portable);assert.equal(resolve({PATH:dir}),portable);
 }finally{fs.rmSync(root,{recursive:true,force:true});}
});
