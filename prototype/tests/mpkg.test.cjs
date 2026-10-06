'use strict';
const test=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),path=require('path'),os=require('os');
const {parse,classify,extract}=require('../tools/mpkg.cjs');
function u(n){const b=Buffer.alloc(4);b.writeUInt32LE(n);return b;}
function str(s){const b=Buffer.from(s);return Buffer.concat([u(b.length),b]);}
function pkg(entries,magic='PKGM0014'){let offset=0;const dirs=[],data=[];for(const [name,b]of entries){dirs.push(str(name),u(offset),u(b.length));data.push(b);offset+=b.length;}return Buffer.concat([str(magic),u(entries.length),...dirs,...data]);}
function sample(video){const media=Buffer.alloc(16);media.writeUInt32BE(16);media.write('ftyp',4);media.write('isom',8);return pkg([
 ['project.json',Buffer.from(JSON.stringify({type:video?'scene':'video',file:video?'clip.mp4':'scene.json'}))],
 [video?'clip.mp4':'scene.json',video?media:Buffer.from('{}')]
]);}
test('synthetic packages classified by entry, not declared type',()=>{for(const video of [true,false]){const b=sample(video),p=parse(b);assert.equal(p.count,2);assert.equal(classify(b,p).kind,video?'video':'unsupported-scene');}});
for(const [variable,count,kind] of [['BELLEWALL_MPKG_VIDEO_FIXTURE',4,'video'],['BELLEWALL_MPKG_SCENE_FIXTURE',41,'unsupported-scene']])
 test('optional real MPKG: '+kind,{skip:!process.env[variable]},()=>{const b=fs.readFileSync(process.env[variable]),p=parse(b);assert.equal(p.count,count);assert.equal(classify(b,p).kind,kind);});
test('unsupported version, truncation, counts and out-of-range extent rejected',()=>{
 assert.throws(()=>parse(pkg([['a',Buffer.from('x')]],'PKGM0099')),/version/);
 assert.throws(()=>parse(Buffer.alloc(3)),/Truncated/);
 assert.throws(()=>parse(Buffer.concat([str('PKGM0014'),u(10001)])),/count/);
 const b=pkg([['a',Buffer.from('x')]]);b.writeUInt32LE(0xffffffff,25);assert.throws(()=>parse(b),/extent/);
});
test('Windows traversal, ADS, reserved names, case collisions, and file-directory collisions rejected',()=>{
 for(const name of ['../x','/x','C:/x','a\\b','a:stream','a/../x','NUL','a./x','a /x','a//x','x\u0000z'])assert.throws(()=>parse(pkg([[name,Buffer.from('x')]])),/path/);
 assert.throws(()=>parse(pkg([['A',Buffer.alloc(1)],['a',Buffer.alloc(1)]])),/Duplicate/);
 assert.throws(()=>parse(pkg([['a',Buffer.alloc(1)],['a/b',Buffer.alloc(1)]])),/collision/);
});
test('overlaps rejected before extraction',()=>{const b=pkg([['a',Buffer.alloc(2)],['b',Buffer.alloc(2)]]);b.writeUInt32LE(1,34);assert.throws(()=>parse(b),/Overlapping/);});
test('an existing output directory cannot be overwritten',()=>{
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'bellewall-mpkg-'));
 try{const input=path.join(dir,'input.mpkg'),out=path.join(dir,'output');fs.writeFileSync(input,sample(true));
  extract(input,out);const before=fs.readFileSync(path.join(out,'project.json'));
  assert.throws(()=>extract(input,out),/Output must not exist/);
  assert.deepEqual(fs.readFileSync(path.join(out,'project.json')),before);
 }finally{const target=fs.realpathSync(dir);assert.equal(path.dirname(target),fs.realpathSync(os.tmpdir()));assert.match(path.basename(target),/^bellewall-mpkg-/);fs.rmSync(target,{recursive:true,force:true});}
});
test('missing or fake MP4 entry is not mistaken for video',()=>{const b=pkg([['project.json',Buffer.from(JSON.stringify({type:'video',file:'scene.json'}))],['scene.json',Buffer.from('{}')]]);assert.equal(classify(b,parse(b)).kind,'unsupported-scene');});
