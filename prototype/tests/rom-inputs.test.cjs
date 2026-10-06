'use strict';
const test=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),os=require('os'),path=require('path');
const {load}=require('../tools/rom-inputs.cjs');
test('release ROM inputs must be explicitly configured',()=>assert.throws(()=>load(''),/BELLEWALL_ROM_CONFIG/));
test('missing and wrong ROM images fail with actionable errors',()=>{
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'bellewall-rom-config-'));
 try{
  const config=path.join(dir,'roms.json');fs.writeFileSync(config,JSON.stringify({nokia603_fp2:'missing.dll'}));
  assert.throws(()=>load(config),/ROM input not found: nokia603_fp2/);
  fs.writeFileSync(path.join(dir,'missing.dll'),'not a ROM');
  assert.throws(()=>load(config),/ROM SHA-256 differs: nokia603_fp2/);
  fs.writeFileSync(config,'{}');assert.throws(()=>load(config),/Missing ROM path: nokia603_fp2/);
 }finally{const target=fs.realpathSync(dir);assert.equal(path.dirname(target),fs.realpathSync(os.tmpdir()));assert.match(path.basename(target),/^bellewall-rom-config-/);fs.rmSync(target,{recursive:true,force:true});}
});
