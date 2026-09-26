'use strict';
// Offline analysis only. Never deploy the uncompressed analysis copy.
const fs=require('fs'),path=require('path');
const root=path.resolve(__dirname,'../..');
const sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474';
function symbols(file){
 if(!fs.existsSync(file))return new Map();
 const d=fs.readFileSync(file),sections=[];
 for(let i=0;i<d.readUInt16LE(48);i++){const p=d.readUInt32LE(32)+i*d.readUInt16LE(46);sections.push({type:d.readUInt32LE(p+4),addr:d.readUInt32LE(p+12),offset:d.readUInt32LE(p+16),size:d.readUInt32LE(p+20),link:d.readUInt32LE(p+24),entsize:d.readUInt32LE(p+36)});}
 const sy=sections.find(s=>s.type===11),st=sections[sy.link],result=new Map();
 for(let p=sy.offset;p<sy.offset+sy.size;p+=sy.entsize){const s=sections[d.readUInt16LE(p+14)],n=st.offset+d.readUInt32LE(p),name=d.toString('utf8',n,d.indexOf(0,n));if(!s||!name)continue;const at=s.offset+d.readUInt32LE(p+4)-s.addr;if(at<0||at+4>d.length)continue;result.set(d.readUInt32LE(at),name);}
 return result;
}
const b=fs.readFileSync(path.join(root,'build/device/rom-fp2/hspsthemeserver-uncompressed.exe'));
if(b.readUInt32LE(0x1c)!==0)throw Error('Expected uncompressed analysis image');
const base=b.readUInt32LE(0x4c),code=b.readUInt32LE(0x64),imp=b.readUInt32LE(0x6c),end=imp+b.readUInt32LE(imp),out=[];
let cursor=imp+4;
for(let i=0;i<b.readUInt32LE(0x54);i++){
 const no=imp+b.readUInt32LE(cursor),count=b.readUInt32LE(cursor+4);
 if(cursor+8+4*count>end||no>=end)throw Error('Import bounds');
 const dll=b.toString('ascii',no,b.indexOf(0,no)),lib=dll.split(/[\[{.]/)[0];
 const names=symbols(path.join(sdk,'epoc32/release/armv5/lib',lib+'.dso'));
 cursor+=8;
 for(let j=0;j<count;j++,cursor+=4){const offset=b.readUInt32LE(cursor),value=b.readUInt32LE(code+offset),ordinal=value&65535;
  out.push({address:base+offset,stub:base+offset-4,dll,ordinal,symbol:names.get(ordinal)||null});
 }
}
const target=path.join(root,'research/evidence/device/fp2-analysis/hspsthemeserver-imports.json');fs.writeFileSync(target,JSON.stringify(out,null,2));
const index=new Map(out.map(x=>[x.stub,x]));
const asm=fs.readFileSync(path.join(root,'build/device/rom-fp2/hspsthemeserver.asm'),'utf8').replace(/(blx\s+0x)([0-9a-f]+)/g,(all,p,hex)=>{const x=index.get(parseInt(hex,16));return x?all+' ; '+x.dll+'!'+(x.symbol||x.ordinal):all;});
fs.writeFileSync(path.join(root,'build/device/rom-fp2/hspsthemeserver-annotated.asm'),asm);
console.log(JSON.stringify({imports:out.length,mapped:out.filter(x=>x.symbol).length,target}));
