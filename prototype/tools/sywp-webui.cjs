'use strict';
const http=require('http'),fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
const {pipeline}=require('stream/promises');
const {encode,display}=require('./sywp.cjs'),mpkg=require('./mpkg.cjs');
function start(port=8765,base=path.resolve(__dirname,'../../build/sywp-webui')){
 base=path.resolve(base);fs.mkdirSync(base,{recursive:true});const token=crypto.randomBytes(24).toString('hex');let busy=false;
 const server=http.createServer(async(req,res)=>{
  const reply=(code,text)=>{if(res.headersSent||res.destroyed)return;res.writeHead(code,{'Content-Type':'text/plain; charset=utf-8','Cache-Control':'no-store'});res.end(text);};let url;try{url=new URL(req.url,'http://127.0.0.1');}catch(e){return reply(400,'Invalid URL');}
  if(!/^127\.0\.0\.1:\d+$/.test(req.headers.host||''))return reply(403,'Invalid host');
  if(req.method==='GET'&&url.pathname==='/bellewall-icon.svg'){res.writeHead(200,{'Content-Type':'image/svg+xml'});return res.end(fs.readFileSync(path.resolve(__dirname,'../../assets/branding/bellewall-icon.svg')));}
  if(req.method==='GET'&&url.pathname==='/'){res.writeHead(200,{'Content-Type':'text/html; charset=utf-8','Cache-Control':'no-store','Content-Security-Policy':"default-src 'self'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; media-src blob:; connect-src 'self'; frame-ancestors 'none'"});return res.end(fs.readFileSync(path.join(__dirname,'sywp-webui.html'),'utf8').replace('__TOKEN__',token));}
  if(req.method!=='POST'||!['/convert','/preview'].includes(url.pathname)||req.headers['x-bellewall-token']!==token)return reply(403,'Invalid request');
  if(busy)return reply(409,'Another conversion is running');busy=true;
  let dir,ownsBusy=true;
  try{
   const ext=url.searchParams.get('ext');if(!['mp4','webm','mov','mkv','mpkg','html'].includes(ext)||url.pathname==='/preview'&&ext!=='mpkg')throw Error('Unsupported input');
   const options=JSON.parse(url.searchParams.get('options')||'{}');if(!options||typeof options!=='object'||Array.isArray(options))throw Error('Invalid options');if(options.encoding==='mp4')throw Error('MP4 wallpaper export is temporarily disabled; use RGB565');dir=fs.mkdtempSync(path.join(base,'job-'));const input=path.join(dir,'input.'+ext),output=path.join(dir,'wallpaper.sywp');
   const fd=fs.openSync(input,'wx');let size=0;try{for await(const b of req){size+=b.length;if(size>512*1024*1024)throw Error('Upload exceeds 512 MiB');fs.writeSync(fd,b);}}finally{fs.closeSync(fd);}
   let result=output,type='application/vnd.bellewall.sywp';
   if(url.pathname==='/preview'){
    const bytes=fs.readFileSync(input),info=mpkg.parse(bytes),kind=mpkg.classify(bytes,info);if(kind.kind!=='video')throw Error('Realtime scene unsupported; pre-render to video');
    const entry=info.entries.find(e=>e.name===kind.entry);result=path.join(dir,'preview.mp4');fs.writeFileSync(result,bytes.subarray(entry.absoluteOffset,entry.absoluteOffset+entry.size));type='video/mp4';
   }else if(ext==='html')fs.writeFileSync(output,encode({format:'sywp',version:1,title:String(options.title||'网页壁纸').slice(0,120),kind:'web',width:Number(options.width??180),height:Number(options.height??320),loop:true,pause:'resume',entry:'index.html',display:{...display,orientation:options.orientation||'auto',fit:options.fit||'cover',background:options.background||'#000000'}},fs.readFileSync(input)));
   else await new Promise((resolve,reject)=>{const child=cp.spawn(process.execPath,[path.join(__dirname,'prepare-sywp.cjs'),input,output,JSON.stringify(options)],{windowsHide:true});let error='';child.stdout.resume();child.stderr.on('data',b=>{error=(error+b).slice(-8192);});child.on('error',reject);child.on('exit',code=>code?reject(Error(error||'Conversion failed')):resolve());});
   busy=false;ownsBusy=false;
   if(!res.destroyed){res.writeHead(200,{'Content-Type':type,'Cache-Control':'no-store','Content-Disposition':'attachment; filename="'+(type==='video/mp4'?'preview.mp4':'wallpaper.sywp')+'"','Content-Length':fs.statSync(result).size});await pipeline(fs.createReadStream(result),res);}
  }catch(e){reply(400,e.message);}finally{if(dir){const resolved=path.resolve(dir);if(path.dirname(resolved)!==base||!path.basename(resolved).startsWith('job-'))throw Error('Unsafe job cleanup');fs.rmSync(resolved,{recursive:true,force:true});}if(ownsBusy)busy=false;}
 });server.requestTimeout=240000;server.listen(port,'127.0.0.1',()=>console.log('BelleWall: http://127.0.0.1:'+server.address().port));return server;
}
if(require.main===module)start(Number(process.env.PORT||8765));module.exports={start};
