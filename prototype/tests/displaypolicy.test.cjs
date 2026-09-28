'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),path=require('path'),cp=require('child_process');
const root=path.resolve(__dirname,'../..'),compiler=process.env.BELLEWALL_HOST_CXX||(process.platform==='win32'?'C:/msys64/usr/bin/g++.exe':'g++');
test('portrait, landscape and E6 fitting remains within source and destination bounds',()=>{
 const out=path.join(root,'build/display-policy-test');fs.mkdirSync(out,{recursive:true});const exe=path.join(out,process.platform==='win32'?'display.exe':'display');const env={...process.env,PATH:path.dirname(compiler)+path.delimiter+process.env.PATH};
 const build=cp.spawnSync(compiler,['-std=c++98','-Wall','-Wextra',path.join(__dirname,'displaypolicy.cpp').replaceAll('\\','/'),'-o',exe.replaceAll('\\','/')],{env,encoding:'utf8',windowsHide:true});assert.equal(build.status,0,build.stdout+build.stderr);
 const run=cp.spawnSync(exe,[],{env,encoding:'utf8',windowsHide:true});assert.equal(run.status,0,run.stdout+run.stderr);
});
