'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),path=require('path'),cp=require('child_process');
const root=path.resolve(__dirname,'../..'),compiler=process.env.BELLEWALL_HOST_CXX||(process.platform==='win32'?'C:/msys64/usr/bin/g++.exe':'g++'),out=path.join(root,'build/video-timing-test');
test('video scheduling supports 10/20/30 fps, rational rates, looping and 30 fps cap',()=>{
    fs.mkdirSync(out,{recursive:true});const exe=path.join(out,process.platform==='win32'?'video-timing.exe':'video-timing');
    const env={...process.env,PATH:path.dirname(compiler)+path.delimiter+process.env.PATH};
    const build=cp.spawnSync(compiler,['-std=c++98','-Wall','-Wextra',path.join(__dirname,'videotiming.cpp').replaceAll('\\','/'),'-o',exe.replaceAll('\\','/')],{cwd:root,env,encoding:'utf8',windowsHide:true});
    assert.equal(build.status,0,String(build.error||'')+build.stdout+build.stderr);
    const run=cp.spawnSync(exe,[],{env,encoding:'utf8',windowsHide:true});
    assert.equal(run.status,0,String(run.error||'')+run.stdout+run.stderr);
});
