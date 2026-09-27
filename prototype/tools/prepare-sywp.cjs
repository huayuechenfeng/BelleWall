'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),os=require('os');
const {encode,MAX,display}=require('./sywp.cjs'),mpkg=require('./mpkg.cjs');
const {inspect:inspectMp4}=require('./mp4-profile.cjs');
const root=path.resolve(__dirname,'../..');
function prepare(input,output,options={}){
 if(fs.existsSync(output))throw Error('Output exists');
 const fps=Number(options.fps??10),frames=Number(options.frames??300),speed=Number(options.speed??1),x=Number(options.x??0.5),y=Number(options.y??0.5);
 const width=Number(options.width??180),height=Number(options.height??320);
 const encoding=options.encoding||'rgb565';if(!['rgb565','mp4'].includes(encoding))throw Error('Invalid output encoding');
 const fit=options.fit||'cover',orientation=options.orientation||'auto',background=options.background||'#000000';
 if(!['cover','contain','stretch'].includes(fit)||!['auto','portrait','landscape'].includes(orientation)||!/^#[0-9a-fA-F]{6}$/.test(background))throw Error('Invalid display policy');
 if(!Number.isInteger(width)||width<2||width>2048||width%2||!Number.isInteger(height)||height<1||height>2048)throw Error('Invalid dimensions');
 if(![10,20,30].includes(fps)||!Number.isInteger(frames)||frames<1||frames>(encoding==='mp4'?100000:Math.floor(MAX/(width*height*2)))||!Number.isFinite(speed)||speed<0.25||speed>4||![x,y].every(n=>Number.isFinite(n)&&n>=0&&n<=1))throw Error('Invalid fps/frames/speed/crop parameters');
 const tmp=fs.mkdtempSync(path.join(os.tmpdir(),'bellewall-sywp-'));
 try{
  let media=path.resolve(input);
  if(path.extname(input).toLowerCase()==='.mpkg'){
   if(fs.statSync(input).size>512*1024*1024)throw Error('MPKG too large');
   const b=fs.readFileSync(input),info=mpkg.parse(b),kind=mpkg.classify(b,info);if(kind.kind!=='video')throw Error('Realtime scene unsupported; pre-render to video');
   const e=info.entries.find(e=>e.name===kind.entry);media=path.join(tmp,'input.mp4');fs.writeFileSync(media,b.subarray(e.absoluteOffset,e.absoluteOffset+e.size));
  }
  if(/\.swf$/i.test(media))throw Error('SWF unsupported');
  const executable=require('./ffmpeg.cjs').resolveFFmpeg();
  const geometry=fit==='cover'?`scale=${width}:${height}:force_original_aspect_ratio=increase,crop=${width}:${height}:(iw-ow)*${x}:(ih-oh)*${y}`:fit==='contain'?`scale=${width}:${height}:force_original_aspect_ratio=decrease,pad=${width}:${height}:(ow-iw)/2:(oh-ih)/2:color=0x${background.slice(1)}`:`scale=${width}:${height}`;
  const converted=path.join(tmp,encoding==='mp4'?'video.mp4':'frames.raw');
  const args=['-nostdin','-v','error','-protocol_whitelist','file,pipe','-i',media,'-map','0:v:0','-an','-sn','-dn','-vf',`setpts=(PTS-STARTPTS)/${speed},fps=${fps},${geometry},setsar=1`,'-frames:v',String(frames),...(encoding==='mp4'?['-c:v','mpeg4','-q:v','4','-g',String(fps),'-bf','0','-pix_fmt','yuv420p','-movflags','+faststart','-f','mp4']:['-pix_fmt','rgb565le','-f','rawvideo']),converted];
  const result=cp.spawnSync(executable,args,{encoding:'utf8',timeout:180000,maxBuffer:1024*1024});if(result.error||result.status)throw Error(String(result.error||result.stderr));
  const p=fs.readFileSync(converted),info=encoding==='mp4'?inspectMp4(p):null;
  const manifest={format:'sywp',version:1,title:String(options.title||path.basename(input)).slice(0,120),kind:'video',width,height,loop:true,pause:'resume',...(info?{container:'mp4',codec:'mpeg4-part2',requiredFeatures:['video-mp4v-v1'],fpsNumerator:info.fpsNumerator,fpsDenominator:info.fpsDenominator,frames:info.frames}:{pixelFormat:'rgb565le',stride:width*2,fpsNumerator:fps,fpsDenominator:1,frames:p.length/(width*height*2)}),display:{...display,orientation,fit,background}};
  fs.writeFileSync(output,encode(manifest,p),{flag:'wx'});return {manifest,ffmpegArgs:args};
 }finally{const tempRoot=path.resolve(os.tmpdir())+path.sep;if(!path.resolve(tmp).startsWith(tempRoot)||!path.basename(tmp).startsWith('bellewall-sywp-'))throw Error('Unsafe temporary directory');fs.rmSync(tmp,{recursive:true,force:true});}
}
if(require.main===module){try{console.log(JSON.stringify(prepare(process.argv[2],process.argv[3],process.argv[4]?JSON.parse(process.argv[4]):{}),null,2));}catch(e){console.error(e.message);process.exitCode=1;}}
module.exports={prepare};
