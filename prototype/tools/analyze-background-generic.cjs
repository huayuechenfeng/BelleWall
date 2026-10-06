'use strict';
// Offline experiment for deriving a desktop background layout without a ROM ID.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),elf=(process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474')+'/epoc32/tools/elf2e32.exe';
const reportAll=process.argv.includes('--report-all');
const inputs=process.argv.slice(2).filter(x=>x!=='--report-all');
const files=inputs.length?inputs:Object.values(require('./rom-inputs.cjs').load());

function run(file,mode){const p=cp.spawnSync(elf,['--e32input='+file,'--dump='+mode],{encoding:'utf8',windowsHide:true,maxBuffer:32*1024*1024});if(p.error||p.status)throw Error(p.error||p.stderr||p.stdout);return p.stdout;}
function unpack(file){const image=fs.readFileSync(file),base=image.readUInt32LE(0x4c),size=image.readUInt32LE(0x30),code=Buffer.alloc(size);let n=0;
 for(const line of run(file,'c').split(/\r?\n/)){const m=line.match(/^([0-9a-f]{6,8}): ((?:[0-9a-f]{8} ){1,8})/i);if(!m)continue;if(parseInt(m[1],16)!==n)throw Error('Code gap');for(const word of m[2].trim().split(' ')){code.writeUInt32LE(parseInt(word,16),n);n+=4;}}
 if(n!==size)throw Error('Code truncation');const exports=new Map([...run(file,'e').matchAll(/Ordinal\s+(\d+):\s*([0-9a-f]{8})/gi)].map(m=>[+m[1],parseInt(m[2],16)]));return {image,base,code,exports};}
function decodeBl(at,code,base){const p=at-base,a=code.readUInt16LE(p),b=code.readUInt16LE(p+2);if((a&0xf800)!==0xf000||(b&0xd000)!==0xd000)return null;
 const s=(a>>10)&1,j1=(b>>13)&1,j2=(b>>11)&1,i1=~(j1^s)&1,i2=~(j2^s)&1;let delta=(s<<24)|(i1<<23)|(i2<<22)|((a&1023)<<12)|((b&2047)<<1);if(s)delta|=~0x1ffffff;return (at+4+delta)>>>0;}
const view=Buffer.from('803010b540698069c04610bd','hex');
const create=Buffer.from('206800f00000290000f0000020602068','hex'),mask=Buffer.from('3ff800f80000ffff00f800003ff83ff8','hex');
const ctor=Buffer.from('0160c83141601831416306641831c563040081634430','hex');
const draw=Buffer.from('f3b5040091b0000000000f90200000000000050000681299','hex'),drawMask=Buffer.from('ffffffffffff00000000ffffffff00000000ffffffffffff','hex');
const sourceOld=fs.readFileSync(path.join(root,'prototype/src/backgroundprofile.h'),'utf8');
const sourceNew=fs.readFileSync(path.join(root,'prototype/src/backgroundprofiles.h'),'utf8');
function sourceArray(source,name){const found=source.match(new RegExp('static const TUint8 '+name+'\\[\\]=\\{([^}]+)\\}'));if(!found)throw Error('Missing '+name);return Buffer.from([...found[1].matchAll(/0x([0-9a-f]{2})/gi)].map(m=>parseInt(m[1],16)));}
const exactProfiles=[
 {name:'603 FP2',view:0x993c,checks:[[0x993c,'KBgFingerprint0',sourceOld],[0x231ce,'KBgFingerprint1',sourceOld],[0x31c02,'KBgFingerprint2',sourceOld],[0x3231c,'KBgFingerprint3',sourceOld]]},
 {name:'E7',view:0x98c4,checks:[[0x98c4,'KE7View',sourceNew],[0x23026,'KE7Create',sourceNew],[0x3194e,'KE7Construct',sourceNew],[0x32078,'KE7Draw',sourceNew]]}
];
function match(code,pos,bytes,bits){if(pos<0||pos+bytes.length>code.length)return false;for(let i=0;i<bytes.length;i++)if((code[pos+i]&bits[i])!==bytes[i])return false;return true;}
const reports=[];
for(const input of files){const file=path.resolve(root,input),{image,base,code,exports}=unpack(file),viewAt=(exports.get(237)&~1),end=(exports.get(277)&~1),matches=[];
 const viewMatches=code.subarray(viewAt-base,viewAt-base+view.length).equals(view);
 for(let at=viewAt;at+create.length<=end;at+=2)if(match(code,at-base,create,mask))matches.push(at);
 const candidates=[];for(const at of matches){const helper=decodeBl(at+2,code,base),factory=decodeBl(at+8,code,base),memberLoad=code.readUInt16LE(at-base),store=code.readUInt16LE(at+12-base),reload=code.readUInt16LE(at+14-base);
  if(helper===null||helper<viewAt||helper>=end||factory===null||factory<base||factory+24>=base+code.length||((store>>6)&31)!==((memberLoad>>6)&31)+1||((reload>>6)&31)!==((memberLoad>>6)&31))continue;
  const f=factory-base;if(code.subarray(f,f+8).toString('hex')!=='70b504000d00b420')continue;
  const constructorCall=decodeBl(factory+16,code,base);if(constructorCall===null)continue;const constructor=constructorCall+6,c=constructor-base;
  if(!match(code,c+6,ctor,Buffer.alloc(ctor.length,0xff)))continue;
  const load=code.readUInt16LE(c+4);if((load&0xff00)!==0x4900)continue;
  const literal=(((constructor+4+4)&~3)+(load&255)*4)>>>0;if(literal<base||literal+4>base+code.length)continue;
  const vtable=code.readUInt32LE(literal-base);if(vtable<base||vtable+0xa8>base+code.length)continue;
  const drawEntry=code.readUInt32LE(vtable+0xa4-base),drawAt=drawEntry&~1;
  const drawOkay=drawEntry%2===1&&match(code,drawAt-base,draw,drawMask);
  candidates.push({create:at,factory,constructor,memberOffset:(code.readUInt16LE(at+12-base)>>6&31)*4,literal,vtable,drawEntry,drawOkay});}
 const rangeOkay=end>viewAt&&end-viewAt>=0x1000&&end-viewAt<0x80000;
 const admitted=rangeOkay&&viewMatches&&candidates.length===1&&candidates[0].drawOkay;
 const reason=!rangeOkay?'invalid code range':!viewMatches?'View export changed':candidates.length!==1?'background chain not unique':!candidates[0].drawOkay?'draw method changed':'verified';
 const exactProfile=exactProfiles.find(profile=>viewAt===profile.view&&profile.checks.every(([address,name,source])=>{const bytes=sourceArray(source,name);return code.subarray(address-base,address-base+bytes.length).equals(bytes);}))?.name||null;
 if(!reportAll&&(!admitted||matches.length!==1||candidates[0].memberOffset!==0x5c))throw Error('Generic layout admission failed for '+file);
 reports.push({file:path.relative(root,file),sha256:crypto.createHash('sha256').update(image).digest('hex'),viewAt,end,viewMatches,viewHex:code.subarray(viewAt-base,viewAt-base+view.length).toString('hex'),createMatches:matches.length,rawMatches:matches.map(at=>({at,bytes:code.subarray(at-base,at-base+16).toString('hex'),helper:decodeBl(at+2,code,base),factory:decodeBl(at+8,code,base)})),exactProfile,admitted,reason,candidates});}
console.log(JSON.stringify(reports,null,2));
