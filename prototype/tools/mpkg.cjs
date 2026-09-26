'use strict';
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function safeName(s) {
 if(typeof s!=='string'||!s||s.includes('\\')||/[\x00-\x1f<>:"|?*]/.test(s)||s.split('/').some(p=>!p||p==='.'||p==='..'||/[. ]$/.test(p)||/^(con|prn|aux|nul|com[0-9]|lpt[0-9])(?:\.|$)/i.test(p)))throw Error('Unsafe path: '+s);
 return s;
}
function parse(b) {
 let p=0;
 function u32(){if(p+4>b.length)throw Error('Truncated directory');const n=b.readUInt32LE(p);p+=4;return n;}
 function str(){const n=u32();if(!n||n>4096||p+n>b.length)throw Error('Invalid string');const bytes=b.subarray(p,p+n),s=bytes.toString('utf8');p+=n;if(!Buffer.from(s).equals(bytes))throw Error('Invalid UTF-8');return s;}
 const magic=str();if(!['PKGM0014','PKGM0020'].includes(magic))throw Error('Unverified version '+magic);
 const count=u32();if(!count||count>10000)throw Error('Invalid count');
 const entries=[],seen=new Set();
 for(let i=0;i<count;i++){const name=safeName(str()),key=name.toLowerCase();if(seen.has(key)||key==='inventory.json')throw Error('Duplicate/reserved path');seen.add(key);entries.push({name,offset:u32(),size:u32()});}
 const base=p;
 for(const e of entries){if(e.offset+e.size>b.length-base)throw Error('Invalid extent');e.absoluteOffset=base+e.offset;e.sha256=hash(b.subarray(e.absoluteOffset,e.absoluteOffset+e.size));}
 const sorted=entries.filter(e=>e.size).slice().sort((a,b)=>a.offset-b.offset);
 for(let i=1;i<sorted.length;i++)if(sorted[i].offset<sorted[i-1].offset+sorted[i-1].size)throw Error('Overlapping extents');
 for(const e of entries){const parts=e.name.toLowerCase().split('/');parts.pop();while(parts.length){if(seen.has(parts.join('/')))throw Error('File/directory collision');parts.pop();}}
 return {magic,count,base,size:b.length,sha256:hash(b),entries};
}
function classify(b,info){
 const pe=info.entries.find(e=>e.name==='project.json');if(!pe||pe.size>1048576)throw Error('Missing/oversized project.json');
 const project=JSON.parse(b.toString('utf8',pe.absoluteOffset,pe.absoluteOffset+pe.size));
 const entry=safeName(project.file),media=info.entries.find(e=>e.name===entry);if(!media)throw Error('Missing project entry');
 const bytes=b.subarray(media.absoluteOffset,media.absoluteOffset+media.size);
 const mp4=bytes.length>=16&&bytes.toString('ascii',4,8)==='ftyp'&&bytes.readUInt32BE(0)>=16&&bytes.readUInt32BE(0)<=bytes.length;
 return {declaredType:project.type,entry,kind:mp4?'video':'unsupported-scene',reason:mp4?'MP4 entry signature verified; codec requires ffprobe':'Scene requires pre-rendering; preview is not wallpaper content'};
}
function extract(input,out){
 const b=fs.readFileSync(input),info=parse(b);info.input=path.resolve(input);info.content=classify(b,info);
 out=path.resolve(out);if(fs.existsSync(out))throw Error('Output must not exist (no overwrites or symlink traversal)');
 for(let p=path.dirname(out);;p=path.dirname(p)){if(fs.existsSync(p)&&fs.lstatSync(p).isSymbolicLink())throw Error('Symlink ancestor');if(path.dirname(p)===p)break;}
 fs.mkdirSync(out,{recursive:true});
 for(const e of info.entries){const target=path.join(out,e.name);fs.mkdirSync(path.dirname(target),{recursive:true});fs.writeFileSync(target,b.subarray(e.absoluteOffset,e.absoluteOffset+e.size),{flag:'wx'});}
 fs.writeFileSync(path.join(out,'inventory.json'),JSON.stringify(info,null,2),{flag:'wx'});return info;
}
function cli(){try{console.log(JSON.stringify(extract(process.argv[2],process.argv[3]||'build/import'),null,2));}catch(e){console.error(e.message);process.exitCode=1;}}
if(require.main===module)cli();
module.exports={parse,classify,extract,safeName,cli};
