'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),path=require('path'),cp=require('child_process');
const root=path.resolve(__dirname,'../..'),compiler=process.env.BELLEWALL_HOST_CXX||(process.platform==='win32'?'C:/msys64/usr/bin/g++.exe':'g++'),out=path.join(root,'build/preparation-transaction-test');
fs.mkdirSync(out,{recursive:true});const exe=path.join(out,process.platform==='win32'?'preparation-transaction.exe':'preparation-transaction');
const env={...process.env,PATH:path.dirname(compiler)+path.delimiter+process.env.PATH};
const build=cp.spawnSync(compiler,['-std=c++98','-Wall','-Wextra',path.join(__dirname,'preparation-transaction.cpp').replaceAll('\\','/'),'-o',exe.replaceAll('\\','/')],{cwd:root,env,encoding:'utf8',windowsHide:true});
assert.equal(build.status,0,String(build.error||'')+build.stdout+build.stderr);
const cases=['interruption after journal flush retries from absent registration','interruption after install reuses completed registration','interrupted two-generation cleanup finishes remaining identity','legacy identity query failure prevents any cleanup mutation','duplicate registration refuses mutation','failed post-install verification retains recovery journal','repeated cleanup of absent registrations is idempotent','cleanup journal and completion protocol remain operation-specific'];
for(let i=0;i<cases.length;i++)test(cases[i],()=>{const r=cp.spawnSync(exe,[String(i+1)],{env,encoding:'utf8',windowsHide:true});assert.equal(r.status,0,String(r.error||'')+r.stdout+r.stderr);});
