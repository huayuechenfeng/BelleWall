'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),rom=path.join(root,'build/device/rom-fp2'),out=path.join(root,'research/evidence/device/fp2-analysis');fs.mkdirSync(out,{recursive:true});
const sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474',gcc=process.env.BELLE_GCCE||'C:/QtSDK/Symbian/tools/gcce4';
const objdump=process.env.BELLE_OBJDUMP||'C:/msys64/opt/devkitpro/devkitARM/bin/arm-none-eabi-objdump.exe';
function run(exe,args){const r=cp.spawnSync(exe,args,{encoding:'utf8',maxBuffer:64*1024*1024});if(r.error||r.status)throw Error(r.error||r.stdout+r.stderr);return r.stdout;}
function symbols(file){if(!fs.existsSync(file))return [];const d=fs.readFileSync(file),sections=[];for(let i=0;i<d.readUInt16LE(48);i++){const p=d.readUInt32LE(32)+i*d.readUInt16LE(46);sections.push({type:d.readUInt32LE(p+4),addr:d.readUInt32LE(p+12),offset:d.readUInt32LE(p+16),size:d.readUInt32LE(p+20),link:d.readUInt32LE(p+24),entsize:d.readUInt32LE(p+36)});}const sy=sections.find(s=>s.type===11),st=sections[sy.link],result=[];for(let p=sy.offset;p<sy.offset+sy.size;p+=sy.entsize){const s=sections[d.readUInt16LE(p+14)],n=st.offset+d.readUInt32LE(p),name=d.toString('utf8',n,d.indexOf(0,n));if(!s||!name)continue;const at=s.offset+d.readUInt32LE(p+4)-s.addr;if(at<0||at+4>d.length)continue;result.push({name,ordinal:d.readUInt32LE(at)});}return result;}
const report=[];
for(const name of (process.argv.includes('--hsps')?['hspsdefrep','hspsclientsession','hspsthemeserver']:['alfdecoderserverclient','alfappservercore','alfrenderstage','alfcompositorrs','aknskins','aknskinsrv','xn3layoutengine'])){
 const file=path.join(rom,name+(name==='hspsthemeserver'?'.exe':'.dll'));
 const header=fs.readFileSync(file),base=header.readUInt32LE(0x4c),size=header.readUInt32LE(0x30),count=header.readUInt32LE(0x5c);
 // Old SDK e32tran rejects some valid FP2 sparse export tables; its validated
 // read-only dump still decompresses them. Reconstruct only the code section.
 const dump=run(sdk+'/epoc32/tools/elf2e32.exe',['--e32input='+file,'--dump=c']),b=Buffer.alloc(size);let written=0;
 for(const line of dump.split(/\r?\n/)){const m=line.match(/^([0-9a-f]{6,8}): ((?:[0-9a-f]{8} ){1,8})/i);if(!m)continue;const address=parseInt(m[1],16);if(address!==written)throw Error('Code dump gap');for(const word of m[2].trim().split(' ')){if(written+4>size)throw Error('Code dump overflow');b.writeUInt32LE(parseInt(word,16),written);written+=4;}}
 if(written!==size)throw Error('Truncated code dump '+name+' '+written+'/'+size);
 const exported=run(sdk+'/epoc32/tools/elf2e32.exe',['--e32input='+file,'--dump=e']),addresses=new Map([...exported.matchAll(/Ordinal\s+(\d+):\s*([0-9a-f]{8})/gi)].map(m=>[Number(m[1]),parseInt(m[2],16)]));
 const codeFile=path.join(rom,name+'.code');fs.writeFileSync(codeFile,b);
 fs.writeFileSync(path.join(rom,name+'.asm'),run(objdump,['-D','-b','binary','-m','arm','-M','force-thumb','--adjust-vma=0x'+base.toString(16),codeFile]));
 const entries=symbols(sdk+'/epoc32/release/armv5/lib/'+name+'.dso').filter(s=>s.ordinal>0&&s.ordinal<=count&&addresses.has(s.ordinal)).map(s=>{const address=addresses.get(s.ordinal),at=(address&~1)-base;return {...s,address,code:b.subarray(at,at+24).toString('hex')};});
 fs.writeFileSync(path.join(out,name+'-symbols.json'),JSON.stringify(entries,null,2));
 fs.writeFileSync(path.join(out,name+'-headers.txt'),run(sdk+'/epoc32/tools/elf2e32.exe',['--e32input='+file,'--dump=hi']));
 report.push({name,sha256:crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex'),bytes:fs.statSync(file).size,base,size,exportCount:count,background:entries.filter(s=>/SetIsBackgroundAnim/.test(s.name))});
}
fs.writeFileSync(path.join(out,process.argv.includes('--hsps')?'hsps-manifest.json':'manifest.json'),JSON.stringify({firmware:'113.010.1506 RM-779',origin:'Read-only phone ROM snapshot using belleprobe.exe',modules:report},null,2));console.log(JSON.stringify(report,null,2));
