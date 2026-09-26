'use strict';
const fs=require('fs'),path=require('path');
function resolveFFmpeg({root=path.resolve(__dirname,'../..'),env=process.env,platform=process.platform}={}){
 const exe=platform==='win32'?'ffmpeg.exe':'ffmpeg';
 const file=p=>{try{return fs.statSync(p).isFile();}catch{return false;}};
 const explicit=env.FFMPEG_BIN?.trim().replace(/^"(.*)"$/,'$1');
 if(explicit){
  const p=path.resolve(explicit),candidate=file(p)?p:path.join(p,exe);
  if(file(candidate))return candidate;
  throw Error('FFMPEG_BIN 指定的位置找不到 FFmpeg：'+explicit+'。请改为 ffmpeg.exe 文件或其所在目录，然后重新启动制作工具。');
 }
 const pathValue=Object.entries(env).find(([k])=>k.toLowerCase()==='path')?.[1]||'';
 const candidates=[path.join(root,'ffmpeg/bin',exe),path.join(root,'bin',exe),
  ...pathValue.split(platform==='win32'?';':':').filter(Boolean).map(p=>path.join(p.replace(/^"(.*)"$/,'$1'),exe)),
  path.join(root,'build/ffmpeg-9.0.1-essentials_build/bin',exe)];
 const found=candidates.find(file);if(found)return found;
 throw Error('未找到 FFmpeg，无法转换视频／MPKG。请将 FFmpeg 解压后完整 bin 目录复制到工具包根目录的 ffmpeg 文件夹，使路径为 ffmpeg/bin/ffmpeg.exe；也可将 FFmpeg 加入 PATH，或设置 FFMPEG_BIN 为程序文件或所在目录。修改环境变量后请关闭旧工具窗口并重新启动。网页壁纸生成不需要 FFmpeg。');
}
module.exports={resolveFFmpeg};
