'use strict';
const test=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),path=require('path');
const {parse,classify,extract}=require('../tools/mpkg.cjs');
function u(n){const b=Buffer.alloc(4);b.writeUInt32LE(n);return b;}
function str(s){const b=Buffer.from(s);return Buffer.concat([u(b.length),b]);}
function pkg(entries,magic='PKGM0014'){let offset=0;const dirs=[],data=[];for(const [name,b]of entries){dirs.push(str(name),u(offset),u(b.length));data.push(b);offset+=b.length;}return Buffer.concat([str(magic),u(entries.length),...dirs,...data]);}
test('both supplied packages classified by entry, not type',()=>{for(const [f,count,kind]of [['C:/Users/chihoko/Downloads/3690417937.mpkg',4,'video'],['C:/Users/chihoko/Desktop/3769551400.mpkg',41,'unsupported-scene']]){const b=fs.readFileSync(f),p=parse(b);assert.equal(p.count,count);assert.equal(classify(b,p).kind,kind);}});
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
test('an existing output directory cannot be overwritten',()=>{assert.throws(()=>extract('C:/Users/chihoko/Downloads/3690417937.mpkg',path.resolve(__dirname)),/must not exist/);});
test('missing or fake MP4 entry is not mistaken for video',()=>{const b=pkg([['project.json',Buffer.from(JSON.stringify({type:'video',file:'scene.json'}))],['scene.json',Buffer.from('{}')]]);assert.equal(classify(b,parse(b)).kind,'unsupported-scene');});
