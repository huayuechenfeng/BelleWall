'use strict';
// Verify the private desktop layouts against immutable, read-only phone ROM
// snapshots before building a multi-layout candidate. No device is contacted.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..');
const sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474';
const elf2e32=path.join(sdk,'epoc32/tools/elf2e32.exe');
const headers={
  old:fs.readFileSync(path.join(root,'prototype/src/backgroundprofile.h'),'utf8'),
  multi:fs.readFileSync(path.join(root,'prototype/src/backgroundprofiles.h'),'utf8')
};
const images=[
  {id:'Nokia 603',file:'archive/build-before-1.0-20260927/device/rom-fp2/xn3layoutengine.dll',hash:'76d6e4451d657f58a09f8de6313d4aac02c74a42927514b5f266df32730f5816',layout:'K603Layout',view:0x993d,end:0x41420,store:0x231da,checks:[[0x993c,'KBgFingerprint0','old'],[0x231ce,'KBgFingerprint1','old'],[0x31c02,'KBgFingerprint2','old'],[0x3231c,'KBgFingerprint3','old']]},
  {id:'Nokia E7',file:'build/e7-diagnosis-20260927/xn3layoutengine.dll',hash:'f2203cbd0e83f7fdc7e69959be32aeea93a1063c8ef2a2cd42feb6e034900c54',layout:'KE7Layout',view:0x98c5,end:0x41074,store:0x23032,checks:[[0x98c4,'KE7View','multi'],[0x23026,'KE7Create','multi'],[0x3194e,'KE7Construct','multi'],[0x32078,'KE7Draw','multi']]}
];
function assert(ok,message){if(!ok)throw Error(message);}
function run(file,option){const result=cp.spawnSync(elf2e32,['--e32input='+file,'--dump='+option],{encoding:'utf8',windowsHide:true,maxBuffer:32*1024*1024});if(result.error||result.status)throw Error(result.error||result.stderr||result.stdout);return result.stdout;}
function array(name,source){const match=source.match(new RegExp('static const TUint8 '+name+'\\[\\]=\\{([^}]+)\\}'));
  assert(match,'Missing fingerprint '+name);return Buffer.from([...match[1].matchAll(/0x([0-9a-f]{2})/gi)].map(x=>parseInt(x[1],16)));}
function layout(name){const match=headers.multi.match(new RegExp('static const TBackgroundLayout '+name+'=\\{([^}]+)\\}'));
  assert(match,'Missing layout '+name);const values=match[1].split(',').map(x=>Number(x.trim()));assert(values.length===5&&values.every(Number.isInteger),'Invalid layout '+name);
  return {id:values[0],viewAddress:values[1],backgroundOffset:values[2],vtableAddress:values[3],drawAddress:values[4]};}
function exportsOf(file){const output=run(file,'e'),result=new Map();for(const match of output.matchAll(/Ordinal\s+(\d+):\s*([0-9a-f]{8})/gi))result.set(Number(match[1]),parseInt(match[2],16));return result;}
function codeOf(file,header){const base=header.readUInt32LE(0x4c),size=header.readUInt32LE(0x30),code=Buffer.alloc(size);let written=0;
  for(const line of run(file,'c').split(/\r?\n/)){const match=line.match(/^([0-9a-f]{6,8}): ((?:[0-9a-f]{8} ){1,8})/i);if(!match)continue;
    assert(parseInt(match[1],16)===written,'Code dump gap '+file);
    for(const word of match[2].trim().split(' ')){assert(written+4<=size,'Code dump overflow '+file);code.writeUInt32LE(parseInt(word,16),written);written+=4;}}
  assert(written===size,'Code dump truncated '+file);return {base,code};}
const reports=[];
for(const image of images){const file=path.join(root,image.file),header=fs.readFileSync(file),hash=crypto.createHash('sha256').update(header).digest('hex');
  assert(hash===image.hash,image.id+' ROM SHA-256 differs');const {base,code}=codeOf(file,header),exports=exportsOf(file),profile=layout(image.layout);
  assert((exports.get(237)&~1)===profile.viewAddress&&exports.get(237)===image.view,image.id+' View export differs');
  assert((exports.get(277)&~1)===image.end,image.id+' range witness differs');
  const span=exports.get(277)&~1;assert(span>=profile.vtableAddress+0xa4+4&&span<0x80000,image.id+' range witness too short');
  const at=address=>{const offset=address-base;assert(offset>=0&&offset<code.length,image.id+' address outside code');return offset;};
  for(const [address,name,section] of image.checks){const bytes=array(name,headers[section]);assert(bytes.length>0&&at(address)+bytes.length<=code.length&&code.subarray(at(address),at(address)+bytes.length).equals(bytes),image.id+' fingerprint '+name+' differs');}
  const operation=code.readUInt16LE(at(image.store));
  assert((operation&0xf800)===0x6000&&(operation&7)===0&&((operation>>3)&7)===4&&(((operation>>6)&31)*4)===profile.backgroundOffset,image.id+' background member store differs');
  assert(code.readUInt32LE(at(profile.vtableAddress+0xa4))===profile.drawAddress,image.id+' draw vtable slot differs');
  reports.push({image:image.id,romSha256:hash,viewExport:exports.get(237),rangeExport:exports.get(277),backgroundOffset:profile.backgroundOffset,vtable:profile.vtableAddress,draw:profile.drawAddress,fingerprints:image.checks.length,verified:true});}
console.log(JSON.stringify({status:'ROM signatures, object store and draw vtable verified',images:reports},null,2));
