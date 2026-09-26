'use strict';
const fs=require('fs'),path=require('path'),crypto=require('crypto'),assert=require('assert/strict'),vm=require('vm');
const root=path.resolve(__dirname,'../..'),bundle=path.resolve(process.argv[2]),out=path.resolve(process.argv[3]);
if(fs.existsSync(out))throw Error('Evidence output exists');fs.mkdirSync(out,{recursive:true});
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
async function main(){
 const entries=fs.readFileSync(path.join(bundle,'SHA256SUMS.txt'),'utf8').trim().split(/\r?\n/);for(const e of entries)assert.equal(sha(fs.readFileSync(path.join(bundle,e.slice(66)))),e.slice(0,64));
 const html=fs.readFileSync(path.join(bundle,'prototype/content/starter.html'),'utf8'),script=html.match(/<script>([\s\S]*?)<\/script>/)[1];
 const drawing=[];const ctx=new Proxy({},{get:(t,p)=>p in t?t[p]:(...args)=>drawing.push([p,...args]),set:(t,p,v)=>(t[p]=v,true)});
 const context={document:{getElementById:()=>({getContext:()=>ctx})},Date,Math};vm.createContext(context);vm.runInContext(script,context,{timeout:1000});drawing.length=0;context.bellewallStep(1000);assert(drawing.some(x=>x[0]==='arc'));assert.equal(typeof context.bellewallStep,'function');
 const sywp=require(path.join(bundle,'prototype/tools/sywp.cjs'));sywp.decode(sywp.encode({format:'sywp',version:1,title:'Starter',kind:'web',width:180,height:320,loop:true,pause:'resume',entry:'index.html',display:sywp.display},Buffer.from(html)));
 process.env.FFMPEG_BIN=process.env.FFMPEG_BIN||path.join(root,'build/ffmpeg-9.0.1-essentials_build/bin');
 const server=require(path.join(bundle,'prototype/tools/sywp-webui.cjs')).start(0,path.join(out,'jobs'));await new Promise(resolve=>server.once('listening',resolve));const cases=[];
 try{const url='http://127.0.0.1:'+server.address().port,page=await(await fetch(url)).text();assert(!page.includes('Nokia 603'));const token=page.match(/'X-BelleWall-Token':'([a-f0-9]+)'/)[1];
  for(const[ext,file]of [['html','prototype/content/starter.html'],['mp4','examples/demo.mp4'],['mpkg','examples/demo-video.mpkg']]){const r=await fetch(url+'/convert?ext='+ext+'&options='+encodeURIComponent(JSON.stringify({title:'Example '+ext,width:180,height:320,frames:10,fps:10})),{method:'POST',headers:{'X-BelleWall-Token':token},body:fs.readFileSync(path.join(bundle,file))});const b=Buffer.from(await r.arrayBuffer());assert.equal(r.status,200,b.toString());const decoded=sywp.decode(b);assert.equal(decoded.manifest.width,180);if(ext!=='html')assert.equal(decoded.manifest.frames,10);cases.push({ext,sha256:sha(b),bytes:b.length});}
 }finally{await new Promise(resolve=>server.close(resolve));}
 const result={status:'passed',hashes:entries.length,starterExecuted:true,conversionCases:cases,scope:'Packaged tool and tutorial sample checks; not a new phone acceptance'};fs.writeFileSync(path.join(out,'result.json'),JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result,null,2));
}
main().catch(e=>{console.error(e);process.exitCode=1;});
