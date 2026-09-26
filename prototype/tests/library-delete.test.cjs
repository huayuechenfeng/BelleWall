'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),path=require('path'),cp=require('child_process');
const root=path.resolve(__dirname,'../..'),compiler=process.env.BELLEWALL_HOST_CXX||(process.platform==='win32'?'C:/msys64/usr/bin/g++.exe':'g++'),out=path.join(root,'build/library-delete-test');
fs.mkdirSync(out,{recursive:true});const exe=path.join(out,process.platform==='win32'?'library-delete.exe':'library-delete');
const env={...process.env,PATH:path.dirname(compiler)+path.delimiter+process.env.PATH};
const build=cp.spawnSync(compiler,['-std=c++98','-Wall','-Wextra',path.join(__dirname,'library-delete.cpp').replaceAll('\\','/'),'-o',exe.replaceAll('\\','/')],{cwd:root,env,encoding:'utf8',windowsHide:true});
assert.equal(build.status,0,String(build.error||'')+build.stdout+build.stderr);
const cases=['active or pending guard prevents every mutation','selection commit failure preserves all files','metadata failure leaves payload and permits retry','payload failure leaves no dangling selection and permits retry','selected wallpaper deletion clears pointer before files','unselected deletion does not change selection'];
for(let i=0;i<cases.length;i++)test(cases[i],()=>{const r=cp.spawnSync(exe,[String(i+1)],{env,encoding:'utf8',windowsHide:true});assert.equal(r.status,0,String(r.error||'')+r.stdout+r.stderr);});
