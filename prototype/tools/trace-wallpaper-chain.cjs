'use strict';
// Read-only RTTI/reference mapping. Addresses are link addresses, not patch targets.
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
const root=path.resolve(__dirname,'../..'),rom=path.join(root,'build/device/rom-fp2'),out=path.join(root,'research/evidence/wallpaper-hook');fs.mkdirSync(out,{recursive:true});
const objdump=process.env.BELLE_OBJDUMP||'C:/msys64/opt/devkitpro/devkitARM/bin/arm-none-eabi-objdump.exe';
const manifest=JSON.parse(fs.readFileSync(path.join(root,'research/evidence/device/fp2-analysis/manifest.json')));
const report={warning:'Static reference candidates only; no process addresses or writable offsets established.',modules:[]};
for(const name of ['xn3layoutengine','aknskinsrv']){
 const metadata=manifest.modules.find(m=>m.name===name),dll=fs.readFileSync(path.join(rom,name+'.dll'));
 if(crypto.createHash('sha256').update(dll).digest('hex')!==metadata.sha256)throw Error('ROM identity changed');
 const b=fs.readFileSync(path.join(rom,name+'.code')),base=metadata.base;
 function refs(address){const found=[];for(let p=0;p+4<=b.length;p+=4)if(b.readUInt32LE(p)===address)found.push(base+p);return found;}
 const names=name==='xn3layoutengine'?['20CXnBackgroundManager','11CXnViewData']:['22CAknsSrvWallpaperCache'];
 const classes=names.map(type=>{const offset=b.indexOf(type);if(offset<0)return {type,found:false};const nameAddress=base+offset;return {type,nameAddress,contexts:b.toString('ascii',Math.max(0,offset-4),offset+type.length+4),typeInfoCandidates:refs(nameAddress).map(ref=>({address:ref-4,refs:refs(ref-4).map(v=>({address:v,words:Array.from({length:24},(_,i)=>v+4+i*4-base+4<=b.length?'0x'+b.readUInt32LE(v+4+i*4-base).toString(16):null)}))}))};});
 const exports=JSON.parse(fs.readFileSync(path.join(root,'research/evidence/device/fp2-analysis',name+'-symbols.json'))).filter(s=>/WallpaperImageL|DecodeWallpaperImageL|AddWallpaperL/.test(s.name));
 for(const s of exports){const start=s.address&~1;const r=cp.spawnSync(objdump,['-D','-b','binary','-m','arm','-M','force-thumb','--adjust-vma=0x'+base.toString(16),'--start-address=0x'+start.toString(16),'--stop-address=0x'+(start+240).toString(16),path.join(rom,name+'.code')],{encoding:'utf8'});if(r.error||r.status)throw Error(r.error||r.stderr);fs.writeFileSync(path.join(out,name+'-ordinal-'+s.ordinal+'.asm.txt'),r.stdout);}
 report.modules.push({name,sha256:metadata.sha256,base,classes,exports});
}
fs.writeFileSync(path.join(out,'static-chain.json'),JSON.stringify(report,null,2));console.log(JSON.stringify(report,null,2));
