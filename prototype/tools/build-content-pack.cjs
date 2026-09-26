'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.join(root,'build/content-pack');fs.mkdirSync(out,{recursive:true});
const sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474';
function run(exe,args){const r=cp.spawnSync(exe,args,{cwd:out,encoding:'utf8'});if(r.error||r.status)throw Error(r.error||r.stderr||r.stdout);}
const input=path.join(root,'build/import-video/wallpaper.mp4'),raw=path.join(out,'video.rgb565');
run(path.join(root,'build/ffmpeg-9.0.1-essentials_build/bin/ffmpeg.exe'),['-nostdin','-v','error','-y','-i',input,'-an','-vf','fps=15,scale=180:320:force_original_aspect_ratio=increase,crop=180:320','-frames:v','30','-pix_fmt','rgb565le','-f','rawvideo',raw]);
const bytes=fs.readFileSync(raw);if(bytes.length!==180*320*2*30)throw Error('Unexpected frame bytes');
const header=Buffer.alloc(24);[0x31565742,180,320,30,15,115200].forEach((n,i)=>header.writeUInt32LE(n,i*4));
const packed=path.join(out,'video-frames.bin');fs.writeFileSync(packed,Buffer.concat([header,bytes]));
fs.writeFileSync(path.join(out,'content.pkg'),'&EN\n#{"BelleWall Content Probe"},(0xE7B31105),0,1,0\n%{"BelleWall Research"}\n:"BelleWall Research"\n"'+packed.replaceAll('\\','/')+'"-"C:\\data\\BelleWall\\video-frames.bin"\n');
require('./ensure-signing.cjs')(root);
run(sdk+'/epoc32/tools/makesis.exe',['content.pkg','content.sis']);
run(sdk+'/epoc32/tools/signsis.exe',['-s','content.sis',path.join(root,'dist/bellecontent-selfsigned.sisx'),path.join(root,'build/signing/prototype.cer'),path.join(root,'build/signing/prototype.key')]);
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const report={source:input,sourceSha256:hash(fs.readFileSync(input)),width:180,height:320,frames:30,fps:15,format:'RGB565LE, 24-byte header, tightly packed rows',durationSeconds:2,packSha256:hash(fs.readFileSync(packed)),distinctFrames:new Set(Array.from({length:30},(_,i)=>hash(bytes.subarray(i*115200,(i+1)*115200)))).size,sisSha256:hash(fs.readFileSync(path.join(root,'dist/bellecontent-selfsigned.sisx')))};
if(report.distinctFrames<2)throw Error('Content is static');
fs.writeFileSync(path.join(root,'research/evidence/wallpaper-hook/content-pack.json'),JSON.stringify(report,null,2));console.log(report);
