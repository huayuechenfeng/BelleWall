'use strict';
const fs=require('fs'),path=require('path'),crypto=require('crypto'),assert=require('assert/strict');
const root=path.resolve(__dirname,'../..'),bundle=process.argv[2]&&path.resolve(process.argv[2]),out=process.argv[3]&&path.resolve(process.argv[3]);
if(!bundle||!out||fs.existsSync(out))throw Error('Supply review directory and new evidence directory');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const files=dir=>fs.readdirSync(dir,{withFileTypes:true}).flatMap(e=>e.isDirectory()?files(path.join(dir,e.name)):[path.join(dir,e.name)]);
async function main(){
 const expected=fs.readFileSync(path.join(bundle,'SHA256SUMS.txt'),'utf8').trim().split(/\r?\n/).map(line=>({name:line.slice(66),sha:line.slice(0,64)}));
 assert.deepEqual(files(bundle).map(p=>path.relative(bundle,p).replaceAll('\\','/')).sort(),[...expected.map(e=>e.name),'SHA256SUMS.txt'].sort());
 for(const e of expected)assert.equal(sha(fs.readFileSync(path.join(bundle,e.name))),e.sha,e.name);
 let links=0;for(const file of files(bundle).filter(f=>f.endsWith('.md')))for(const m of fs.readFileSync(file,'utf8').matchAll(/\]\(([^)]+)\)/g)){if(/^(https?:|#)/.test(m[1]))continue;assert(fs.existsSync(path.resolve(path.dirname(file),decodeURIComponent(m[1].split('#')[0]))),'Broken link '+file+': '+m[1]);links++;}
 const manifest=JSON.parse(fs.readFileSync(path.join(bundle,'review-manifest.json'),'utf8'));
 for(const e of manifest.files)if(e.file.startsWith('phone/'))assert.equal(sha(fs.readFileSync(path.join(bundle,e.file))),sha(fs.readFileSync(path.join(root,e.source))));
 fs.mkdirSync(out,{recursive:true});
 if(process.argv[4]==='--integrity-only'){const result={status:'passed',bundle:path.relative(root,bundle).replaceAll('\\','/'),hashedFiles:expected.length,localLinks:links,phonePackages:'byte-identical to accepted checkpoint',scope:'Integrity and links only; unchanged PC tools retain prior conversion evidence.'};fs.writeFileSync(path.join(out,'result.json'),JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result,null,2));return;}
 const sywp=require(path.join(bundle,'pc/prototype/tools/sywp.cjs'));
 const {start}=require(path.join(bundle,'pc/prototype/tools/sywp-webui.cjs'));
 process.env.FFMPEG_BIN=process.env.FFMPEG_BIN||path.join(root,'build/ffmpeg-9.0.1-essentials_build/bin');
 const jobs=path.join(out,'jobs'),server=start(0,jobs);await new Promise((resolve,reject)=>{server.once('listening',resolve);server.once('error',reject);});
 const cases=[];
 try{
  const url='http://127.0.0.1:'+server.address().port,html=await(await fetch(url)).text(),token=html.match(/'X-BelleWall-Token':'([a-f0-9]+)'/)[1];
  const samples=path.join(root,'build/product-assets');
  for(const [ext,file] of [['html',path.join(bundle,'pc/prototype/content/clock.html')],['mp4',path.join(samples,'demo.mp4')],['mpkg',path.join(samples,'preview.mpkg')]]){
   const options={title:'BelleWall PC 制作验证 '+ext,width:180,height:320,fps:10,frames:10,fit:'cover',speed:1};
   const response=await fetch(url+'/convert?ext='+ext+'&options='+encodeURIComponent(JSON.stringify(options)),{method:'POST',headers:{'X-BelleWall-Token':token},body:fs.readFileSync(file)});
   const bytes=Buffer.from(await response.arrayBuffer());assert.equal(response.status,200,bytes.toString());const result=sywp.decode(bytes);assert.equal(result.manifest.title,options.title);assert.equal(result.manifest.width,180);assert.equal(result.manifest.height,320);
   if(ext!=='html'){assert.equal(result.manifest.frames,10);assert.equal(result.manifest.fpsNumerator,10);assert.equal(result.payload.length,1152000);}else assert.equal(result.manifest.kind,'web');
   fs.writeFileSync(path.join(out,'generated-'+ext+'.sywp'),bytes);cases.push({input:ext,bytes:bytes.length,sha256:sha(bytes),manifest:result.manifest});
  }
  const denied=await fetch(url+'/convert?ext=html',{method:'POST',body:'x'});assert.equal(denied.status,403);await denied.text();
  const unsupported=await fetch(url+'/convert?ext=swf',{method:'POST',headers:{'X-BelleWall-Token':token},body:'x'});assert.equal(unsupported.status,400);await unsupported.text();
  for(let i=0;i<20&&fs.readdirSync(jobs).length;i++)await new Promise(resolve=>setTimeout(resolve,50));assert.deepEqual(fs.readdirSync(jobs),[]);
 }finally{await new Promise(resolve=>server.close(resolve));}
 const result={status:'passed',bundle:path.relative(root,bundle).replaceAll('\\','/'),hashedFiles:expected.length,localLinks:links,phonePackages:'byte-identical to accepted checkpoint',cases,negativeCases:{missingToken:403,unsupportedSWF:400},temporaryJobsRemaining:0,scope:'Packaged HTTP conversion and file integrity. Not browser visual QA or phone import acceptance.'};
 fs.writeFileSync(path.join(out,'result.json'),JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result,null,2));
}
main().catch(e=>{console.error(e);process.exitCode=1;});
