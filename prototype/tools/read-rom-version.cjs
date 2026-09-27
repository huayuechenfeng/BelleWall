'use strict';
// Read version resources from a previously validated CORE ROFS inventory.
const fs=require('fs'),path=require('path');
function readVersionResources(directory){
 const inventory=JSON.parse(fs.readFileSync(path.join(directory,'inventory.json'),'utf8'));
 const blocks=JSON.parse(fs.readFileSync(path.join(directory,'blocks.json'),'utf8'));
 const image=fs.readFileSync(inventory.input);
 function read(entry){const out=Buffer.alloc(entry.size);let written=0;
  for(const block of blocks.filter(b=>b.type===0x17)){
   const from=inventory.rofs.address+entry.addr+written;
   if(from<block.addr||from>=block.addr+block.size)continue;
   const count=Math.min(entry.size-written,block.addr+block.size-from);
   image.copy(out,written,block.data+from-block.addr,block.data+from-block.addr+count);written+=count;
   if(written===entry.size)return out;
  }
  throw Error('Version resource gap '+entry.name);
 }
 const resources={};
 for(const name of ['platform.txt','sw.txt','product.txt']){
  const matches=inventory.entries.filter(e=>e.name.toLowerCase()==='resource/versions/'+name);
  if(matches.length!==1)throw Error('Expected one '+name);
  const value=read(matches[0]);
  const encoding=value[0]===0xff&&value[1]===0xfe?'utf16le':'utf8';
  resources[name]=value.toString(encoding).replace(/\u0000/g,'').trim();
 }
 return resources;
}
module.exports={readVersionResources};
if(require.main===module){
 for(const [name,value] of Object.entries(readVersionResources(path.resolve(process.argv[2])))){
  console.log(name+': '+JSON.stringify(value));
 }
}
