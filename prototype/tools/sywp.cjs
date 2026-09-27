'use strict';
// SYWP 1: fixed header + UTF-8 JSON manifest + one bounded payload.
const fs=require('fs'),crypto=require('crypto');
const {inspect:inspectMp4}=require('./mp4-profile.cjs');
const MAX=256*1024*1024, HEADER=64;
const sha=b=>crypto.createHash('sha256').update(b).digest();
function integer(n,min,max){if(!Number.isInteger(n)||n<min||n>max)throw Error('Integer out of range');return n;}
function validate(m,p){
 if(!m||m.format!=='sywp'||m.version!==1||!['video','web'].includes(m.kind)||typeof m.title!=='string'||!m.title.trim()||m.title.length>120)throw Error('Invalid manifest');
 integer(m.width,2,2048);integer(m.height,1,2048);if(m.width%2||m.loop!==true||m.pause!=='resume')throw Error('Unsupported display/playback profile');
 if(!m.display||!['auto','portrait','landscape'].includes(m.display.orientation)||!['cover','contain','stretch'].includes(m.display.fit)||m.display.rotation!=='follow-display'||!/^#[0-9a-fA-F]{6}$/.test(m.display.background))throw Error('Invalid display policy');
 const mp4=m.kind==='video'&&m.container==='mp4'&&m.codec==='mpeg4-part2';
 if((mp4&&m.requiredFeatures===undefined)||(m.requiredFeatures!==undefined&&(!Array.isArray(m.requiredFeatures)||JSON.stringify(m.requiredFeatures)!==(mp4?'["video-mp4v-v1"]':'[]'))))throw Error('Unknown required feature');
 if(m.kind==='video'){
  if(mp4){
   if(m.pixelFormat!==undefined||m.stride!==undefined||!p.length)throw Error('Invalid MP4 video profile');
   const info=inspectMp4(p);if(info.width!==m.width||info.height!==m.height||info.frames!==m.frames||info.fpsNumerator!==m.fpsNumerator||info.fpsDenominator!==m.fpsDenominator)throw Error('MP4 manifest/media mismatch');
  }else if(m.pixelFormat!=='rgb565le'||m.stride!==m.width*2||m.container!==undefined||m.codec!==undefined)throw Error('Unsupported video profile');
  integer(m.fpsNumerator,1,60000);integer(m.fpsDenominator,1,1001);if(m.fpsNumerator/m.fpsDenominator>60||m.fpsNumerator/m.fpsDenominator<1)throw Error('Frame rate outside 1–60 fps');
  integer(m.frames,1,mp4?100000:Math.floor(MAX/(m.stride*m.height)));if(!mp4&&p.length!==m.frames*m.stride*m.height)throw Error('Frame extent mismatch');
 }else{
  if(m.entry!=='index.html'||p.length>256*1024||!p.length||!Buffer.from(p.toString('utf8')).equals(p)||!p.toString('utf8').includes('bellewallStep'))throw Error('Invalid self-contained WebKit entry');
 }
}
function encode(m,p){validate(m,p);const j=Buffer.from(JSON.stringify(m));if(j.length>16384||p.length>MAX)throw Error('Package limit');const h=Buffer.alloc(HEADER);h.write('SYWP');h.writeUInt32LE(1,4);h.writeUInt32LE(HEADER,8);h.writeUInt32LE(j.length,12);h.writeUInt32LE(p.length,16);sha(Buffer.concat([j,p])).copy(h,24);return Buffer.concat([h,j,p]);}
function decode(b){
 if(b.length<HEADER||!b.subarray(0,4).equals(Buffer.from('SYWP'))||b.readUInt32LE(4)!==1||b.readUInt32LE(8)!==HEADER||b.readUInt32LE(20)!==0||b.subarray(56,64).some(x=>x))throw Error('Invalid SYWP header');
 const n=b.readUInt32LE(12),size=b.readUInt32LE(16);if(!n||n>16384||size>MAX||HEADER+n+size!==b.length)throw Error('Invalid package extent');
 if(!sha(b.subarray(HEADER)).equals(b.subarray(24,56)))throw Error('SHA-256 mismatch');
 const j=b.subarray(HEADER,HEADER+n);if(!Buffer.from(j.toString('utf8')).equals(j))throw Error('Invalid UTF-8');
 const manifest=JSON.parse(j),payload=b.subarray(HEADER+n);validate(manifest,payload);return {manifest,payload};
}
function read(file){const size=fs.statSync(file).size;if(size>MAX+HEADER+16384)throw Error('Package too large');return decode(fs.readFileSync(file));}
function frameStream(m,p){validate(m,p);if(m.kind!=='video'||m.pixelFormat!=='rgb565le')throw Error('RGB565 video required');const h=Buffer.alloc(48);[0x32565742,48,m.width,m.height,m.stride,1,m.fpsNumerator,m.fpsDenominator,m.frames,m.stride*m.height,48,p.length].forEach((v,i)=>h.writeUInt32LE(v,4*i));return Buffer.concat([h,p]);}
const display={orientation:'auto',fit:'cover',rotation:'follow-display',background:'#000000'};
if(require.main===module){try{const [cmd,input,output]=process.argv.slice(2);if(cmd==='inspect')console.log(JSON.stringify(read(input).manifest,null,2));else if(cmd==='web'){const p=fs.readFileSync(input),options=process.argv[5]?JSON.parse(process.argv[5]):{};fs.writeFileSync(output,encode({format:'sywp',version:1,title:'WebKit wallpaper',kind:'web',width:options.width||180,height:options.height||320,loop:true,pause:'resume',entry:'index.html',display:{...display,...options.display}},p),{flag:'wx'});}else throw Error('Usage: sywp.cjs inspect file.sywp | web index.html output.sywp [options-json]');}catch(e){console.error(e.message);process.exitCode=1;}}
module.exports={encode,decode,read,validate,frameStream,MAX,display};
