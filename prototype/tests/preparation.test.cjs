'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),path=require('path'),cp=require('child_process');
const root=path.resolve(__dirname,'../..'),compiler=process.env.BELLEWALL_HOST_CXX||(process.platform==='win32'?'C:/msys64/usr/bin/g++.exe':'g++'),out=path.join(root,'build/preparation-test');
fs.mkdirSync(out,{recursive:true});const exe=path.join(out,process.platform==='win32'?'preparation.exe':'preparation');
const env={...process.env,PATH:path.dirname(compiler)+path.delimiter+process.env.PATH};
const build=cp.spawnSync(compiler,['-std=c++98','-Wall','-Wextra',path.join(__dirname,'preparation.cpp').replaceAll('\\','/'),'-o',exe.replaceAll('\\','/')],{cwd:root,env,encoding:'utf8',windowsHide:true});
assert.equal(build.status,0,String(build.error||'')+build.stdout+build.stderr);
const cases=['matching preparation handshake and clean exit permit playback','old helper zero exit without preparation handshake is rejected','panic and forced termination cannot grant readiness','registration errors cannot grant readiness','registration and removal journals preserve fixed ABI','corrupt or foreign preparation journals fail closed'];
for(let i=0;i<cases.length;i++)test(cases[i],()=>{const r=cp.spawnSync(exe,[String(i+1)],{env,encoding:'utf8',windowsHide:true});assert.equal(r.status,0,String(r.error||'')+r.stdout+r.stderr);});
