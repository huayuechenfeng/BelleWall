'use strict';
// Check current entrypoints only. Archived reports are historical evidence.
const fs=require('fs'),path=require('path'),crypto=require('crypto'),assert=require('assert/strict');
const root=path.resolve(__dirname,'../..'),read=p=>fs.readFileSync(path.join(root,p),'utf8');
const state={sisVersion:'1.0.0'};
const files=['README.md',...fs.readdirSync(path.join(root,'doc')).filter(n=>n.endsWith('.md')).map(n=>'doc/'+n)];
let links=0;
for(const file of files){
 for(const m of read(file).matchAll(/\]\(([^)]+)\)/g)){
  let target=m[1];if(/^(https?:|mailto:|#)/.test(target))continue;
  target=decodeURIComponent(target.split('#')[0]);
  assert(fs.existsSync(path.resolve(root,path.dirname(file),target)),file+': broken link '+target);++links;
 }
}
for(const file of ['prototype/tools/package.cjs','prototype/tools/build-native-wallpaper.cjs','prototype/tools/package-session-helper.cjs'])
 assert(read(file).includes("process.env.BELLEWALL_PRODUCT_VERSION||'"+state.sisVersion+"'"),file+': default SIS version drift');
assert(read('prototype/src/sywpimport.h').includes('BelleWall 1.0'),'UI version drift');
assert(!read('prototype/tools/sywp-webui.html').includes('Nokia 603'),'Product label must not name device');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
let hashes=0;
for(const line of read('prototype/baseline/SHA256SUMS.txt').trim().split(/\r?\n/)){
 assert.equal(sha(fs.readFileSync(path.join(root,'prototype/baseline',line.slice(66)))),line.slice(0,64));++hashes;
}
const archive='archive/docs-before-consolidation-20260926/manifest.json';
if(fs.existsSync(path.join(root,archive)))for(const item of JSON.parse(read(archive)))
 assert.equal(sha(fs.readFileSync(path.join(root,item.archive))),item.sha256,'Archived original changed: '+item.original);
console.log(JSON.stringify({status:'passed',documents:files.length,localLinks:links,installedCheckpoint:state.installedCheckpoint,installedHashes:hashes,sisVersion:state.sisVersion,scope:'Links, version declarations, installed evidence and frozen hashes; not proof of all prose semantics'},null,2));
