'use strict';
// Read-only, sample-scoped FPSX 0x17/0x27/0x28 and ROFS v0x200 parser.
// No ROM patching, signature bypass, guessed ordinal calls, or firmware execution.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const input=process.argv[2]||JSON.parse(fs.readFileSync('research/evidence/rom-inventory.json'))[0].file;
const out=path.resolve(process.argv[3]||'build/rom-603');fs.mkdirSync(out,{recursive:true});
const b=fs.readFileSync(input);let p=b.readUInt32BE(1)+5;const blocks=[];
if(b[0]!==0xb2)throw Error('Unsupported FPSX signature');
while(p<b.length){
 const type=b[p+2],h=b[p+3];
 if(!((type===0x17&&h===14)||(type===0x27&&h===45)||(type===0x28&&h===67)))throw Error('Unsupported FPSX block '+p);
 const n=type===0x28?45:h,size=b.readUInt32BE(p+4+n-8),addr=b.readUInt32BE(p+4+n-4),data=p+5+h;
 if(data+size>b.length)throw Error('FPSX bounds');blocks.push({offset:p,type,headerSize:h,size,addr,data});p=data+size;
}
// Search only CODE payloads for a structurally valid ROFS header.
const candidates=[];
for(const block of blocks.filter(x=>x.type===0x17)){
 let i=block.data-1;while((i=b.indexOf(Buffer.from('ROFS'),i+1))>=0&&i<block.data+block.size){
  if(i+48<=block.data+block.size&&b[i+4]===48&&b.readUInt16LE(i+6)===0x200&&b.readUInt32LE(i+8)===48)candidates.push({offset:i,address:block.addr+i-block.data});
 }
}
if(candidates.length!==1)throw Error('Expected one validated CORE ROFS, found '+candidates.length);
const candidate=candidates[0],length=b.readUInt32LE(candidate.offset+36);
if(length<48||length>256*1024*1024)throw Error('ROFS size');
const image=Buffer.alloc(length,255);let covered=0;
const pieces=blocks.filter(x=>x.type===0x17&&x.addr+x.size>candidate.address&&x.addr<candidate.address+length).sort((a,b)=>a.addr-b.addr);
for(const block of pieces){const start=Math.max(block.addr,candidate.address),end=Math.min(block.addr+block.size,candidate.address+length);if(start-candidate.address!==covered)throw Error('ROFS gap/overlap');b.copy(image,start-candidate.address,block.data+start-block.addr,block.data+end-block.addr);covered+=end-start;}
if(covered!==length)throw Error('Incomplete ROFS');
const u32=o=>image.readUInt32LE(o),u16=o=>image.readUInt16LE(o),align=n=>(n+3)&~3;
const entries=[],seen=new Set();
function entry(o){const size=u16(o),nameOffset=image[o+18],nameLength=image[o+29];if(size<30||o+size>length||nameOffset<30||nameOffset+nameLength*2>size)throw Error('ROFS entry bounds '+o);const name=image.toString('utf16le',o+nameOffset,o+nameOffset+nameLength*2);if(!name||/[\\/:\x00]/.test(name)||name==='.'||name==='..')throw Error('ROFS name');return {name,entryOffset:o,entrySize:size,size:u32(o+20),addr:u32(o+24),attributes:image[o+19],extra:image[o+28]};}
function dir(o,prefix){
 if(seen.has(o)||seen.size>10000||o<48||o+12>length)throw Error('ROFS directory cycle/bounds');seen.add(o);
 const size=u16(o),first=image[o+3],fileAddr=u32(o+4),fileSize=u32(o+8);
 if(first<12||first>size||o+size>length||fileAddr+fileSize>length)throw Error('ROFS directory extent');
 for(let j=o+first;j<o+size;){const e=entry(j);dir(e.addr,prefix+e.name+'/');j+=align(e.entrySize);}
 for(let j=fileAddr;j<fileAddr+fileSize;){const e=entry(j);e.name=prefix+e.name;entries.push(e);j+=align(e.entrySize);}
}
dir(u32(8),'');
const selected=[];
for(const e of entries.filter(x=>/^sys\/bin\/(alfdecoderserverclient|alfserver|alfappservercore|alfcompositorrs|alfclient|alfrenderstage|alf|xn3layoutengine|homescreen|QtWebKit|QtCore|QtGui|QtNetwork|libEGL|libGLESv1_CM|libstdcpp|libc|mediaclientvideo|backgroundanimhost|bga_reference_plugin)\.(dll|exe)$/i.test(x.name))){
 if(e.addr+e.size>length||!e.addr)throw Error('ROFS selected file bounds '+e.name);
 const data=image.subarray(e.addr,e.addr+e.size),target=path.join(out,path.basename(e.name));fs.writeFileSync(target,data);selected.push({...e,sha256:sha(data),header:data.subarray(0,40).toString('hex'),extracted:target});
}
const report={input,inputSha256:sha(b),fpsxBlockCount:blocks.length,rofs:candidate,imageSize:length,rofsSha256:sha(image),directoryCount:seen.size,fileCount:entries.length,selected,entries,limitations:'Checks structure and contiguous CODE ranges; does not authenticate FPSX certificates/checksums. ROFS file extraction is not runtime ABI or service behavior proof.'};
fs.writeFileSync(path.join(out,'inventory.json'),JSON.stringify(report,null,2));fs.writeFileSync(path.join(out,'blocks.json'),JSON.stringify(blocks,null,2));console.log(JSON.stringify({...report,entries:undefined},null,2));



