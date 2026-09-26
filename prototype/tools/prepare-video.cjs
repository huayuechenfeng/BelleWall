'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const {extract}=require('./mpkg.cjs');
const input=path.resolve(process.argv[2]||'C:/Users/chihoko/Downloads/3690417937.mpkg');
const out=path.resolve(process.argv[3]||'build/prepared-video');
const bin=process.env.FFMPEG_BIN||path.resolve('build/ffmpeg-9.0.1-essentials_build/bin');
if(fs.existsSync(out))throw Error('Output must be new');fs.mkdirSync(out,{recursive:true});
let video=input,inventory;
if(/\.mpkg$/i.test(input)){inventory=extract(input,path.join(out,'extracted'));if(inventory.content.kind!=='video')throw Error('Unsupported realtime scene: pre-render first; preview will not be played');video=path.join(out,'extracted',inventory.content.entry);}
function run(name,args){const r=cp.spawnSync(path.join(bin,name+'.exe'),args,{encoding:'utf8',maxBuffer:8*1024*1024});if(r.error)throw r.error;if(r.status)throw Error(r.stderr);return r;}
function probe(file){return JSON.parse(run('ffprobe',['-v','error','-show_streams','-show_format','-of','json',file]).stdout);}
const before=probe(video),target=path.join(out,'sample.mp4');
if(!before.streams.some(s=>s.codec_type==='video'))throw Error('No video stream');
const args=['-nostdin','-v','warning','-i',video,'-map','0:v:0','-an','-sn','-dn','-vf','scale=180:320:force_original_aspect_ratio=decrease:force_divisible_by=2,pad=180:320:(ow-iw)/2:(oh-ih)/2,setsar=1,fps=15','-c:v','libx264','-profile:v','baseline','-level:v','1.2','-pix_fmt','yuv420p','-b:v','240k','-maxrate','300k','-bufsize','600k','-g','30','-bf','0','-refs','1','-movflags','+faststart','-map_metadata','-1',target];
const result=run('ffmpeg',args),after=probe(target);
const s=after.streams[0];if(after.streams.length!==1||s.codec_name!=='h264'||s.width!==180||s.height!==320||s.avg_frame_rate!=='15/1'||s.pix_fmt!=='yuv420p'||s.level!==12||s.has_b_frames!==0)throw Error('Transcode validation failed');
run('ffmpeg',['-nostdin','-v','error','-xerror','-i',target,'-map','0:v:0','-f','null','-']);
const report={input,package:inventory&&{magic:inventory.magic,sha256:inventory.sha256,content:inventory.content},before,after,command:{exe:path.join(bin,'ffmpeg.exe'),args},encoder:run('ffmpeg',['-version']).stdout.split('\n')[0],warnings:result.stderr,sha256:crypto.createHash('sha256').update(fs.readFileSync(target)).digest('hex'),validation:'PC full decode passed; Belle MMF and native desktop playback NOT tested'};
fs.writeFileSync(path.join(out,'report.json'),JSON.stringify(report,null,2));console.log(JSON.stringify({target,sha256:report.sha256,validation:report.validation},null,2));
