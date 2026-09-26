'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474',gcc=process.env.BELLE_GCCE||'C:/QtSDK/Symbian/tools/gcce4';
const root=path.resolve(__dirname,'../..'),rom=path.join(root,'build/rom-603'),out=path.join(root,'research/evidence/prototype');fs.mkdirSync(out,{recursive:true});
function run(exe,args){const r=cp.spawnSync(exe,args,{encoding:'utf8',maxBuffer:32*1024*1024});if(r.error)throw r.error;if(r.status)throw Error(r.stdout+r.stderr);return r.stdout;}
const dso=fs.readFileSync(sdk+'/epoc32/release/armv5/lib/alfdecoderserverclient.dso');
const shoff=dso.readUInt32LE(32),shentsize=dso.readUInt16LE(46),shnum=dso.readUInt16LE(48),sections=[];
for(let i=0;i<shnum;i++){const p=shoff+i*shentsize;sections.push({type:dso.readUInt32LE(p+4),addr:dso.readUInt32LE(p+12),offset:dso.readUInt32LE(p+16),size:dso.readUInt32LE(p+20),link:dso.readUInt32LE(p+24),entsize:dso.readUInt32LE(p+36)});}
const symbols=sections.find(s=>s.type===11),strings=sections[symbols.link],sdkEntries=[];
for(let p=symbols.offset;p<symbols.offset+symbols.size;p+=symbols.entsize){const start=strings.offset+dso.readUInt32LE(p),name=dso.toString('utf8',start,dso.indexOf(0,start)),section=sections[dso.readUInt16LE(p+14)];if(!section||!name)continue;const value=dso.readUInt32LE(p+4);if(!/CAlfCompositionSource/.test(name))continue;sdkEntries.push({symbol:name,ordinal:dso.readUInt32LE(section.offset+value-section.addr)});}
const target=path.join(rom,'alfdecoderserverclient.dll'),plain=path.join(rom,'alf-client-uncompressed.dll');
run(sdk+'/epoc32/tools/elf2e32.exe',['--e32input='+target,'--output='+plain,'--uncompressed','--e32tran']);
const b=fs.readFileSync(plain),codeOffset=b.readUInt32LE(0x64),codeBase=b.readUInt32LE(0x4c),exportOffset=b.readUInt32LE(0x58),count=b.readUInt32LE(0x5c);
for(const e of sdkEntries){if(e.ordinal>count)throw Error('Missing target export');e.targetAddress=b.readUInt32LE(exportOffset+4*(e.ordinal-1));}
const bg=sdkEntries.find(e=>e.symbol==='_ZN21CAlfCompositionSource19SetIsBackgroundAnimEi');
const address=bg.targetAddress&~1,offset=codeOffset+address-codeBase,code=b.subarray(offset,offset+16);
// Check exact observed Thumb sequence before interpreting its PC-relative literal.
if(code.toString('hex')!=='13b504230f4901aa891cfff794fe1cbd')throw Error('Different target implementation; manual review required');
const literalAddress=((address+4+4)&~3)+60;
const literal=b.readUInt32LE(codeOffset+literalAddress-codeBase),operation=literal+2;
if(operation!==15011)throw Error('Background operation differs');
const slice=path.join(rom,'background-export.bin');fs.writeFileSync(slice,code);
const assembly=run(gcc+'/bin/arm-none-symbianelf-objdump.exe',['-D','-b','binary','-m','arm','-M','force-thumb','--adjust-vma=0x'+address.toString(16),slice]);
fs.writeFileSync(path.join(out,'603-background-export-disassembly.txt'),assembly);
const report={firmware:'Nokia 603 RM-779 111.020.0310',targetSha256:crypto.createHash('sha256').update(fs.readFileSync(target)).digest('hex'),sdkDsoSha256:crypto.createHash('sha256').update(dso).digest('hex'),sdkEntries,background:{...bg,code:code.toString('hex'),literalAddress,operation,payloadBytes:4},verdict:'Static agreement of SDK ordinal, target export and operation payload. Not verification of full class ABI, live IPC acceptance, wallpaper transparency or compositor behavior.'};
fs.writeFileSync(path.join(out,'603-abi.json'),JSON.stringify(report,null,2));
for(const name of ['alfdecoderserverclient.dll','alfappservercore.dll','alfrenderstage.dll','alfcompositorrs.dll','QtCore.dll','QtGui.dll','QtNetwork.dll','QtWebKit.dll']){
 fs.writeFileSync(path.join(out,name+'.headers-imports.txt'),run(sdk+'/epoc32/tools/elf2e32.exe',['--e32input='+path.join(rom,name),'--dump=hi']));
}
const inventory=JSON.parse(fs.readFileSync(path.join(rom,'inventory.json')));delete inventory.entries;fs.writeFileSync(path.join(out,'603-rofs-summary.json'),JSON.stringify(inventory,null,2));
const hostImports=run(sdk+'/epoc32/tools/elf2e32.exe',['--e32input='+path.join(root,'dist/bellewall.exe'),'--dump=i']);
const checks=[];
for(const match of hostImports.matchAll(/(\d+) imports from ([^\r\n]+)\r?\n([\s\S]*?)(?=\d+ imports from|$)/g)){
 const library=match[2],name=library.split('{')[0]+'.dll',targetFile=path.join(rom,name);
 const ordinals=[...new Set(match[3].split(/\r?\n/).map(s=>s.trim()).filter(s=>/^\d+$/.test(s)).map(Number))];
 if(!fs.existsSync(targetFile)){checks.push({library,status:'Not extracted; live loader check still required',ordinals});continue;}
 const exported=run(sdk+'/epoc32/tools/elf2e32.exe',['--e32input='+targetFile,'--dump=e']);
 const missing=ordinals.filter(n=>{const m=exported.match(new RegExp('Ordinal\\s+'+n+':\\s*([^\\r\\n]+)'));return !m||!/^[0-9a-f]{8}$/i.test(m[1].trim());});
 checks.push({library,ordinals,missing,status:missing.length?'MISSING EXPORT':'All requested ordinals have target exports (semantics not tested)'});
 if(missing.length)throw Error('Target missing imported ordinals '+library+': '+missing.join(','));
}
fs.writeFileSync(path.join(out,'603-host-import-check.json'),JSON.stringify(checks,null,2));
console.log(JSON.stringify(report.background,null,2));
