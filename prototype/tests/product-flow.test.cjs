'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),path=require('path'),os=require('os'),cp=require('child_process');
const {prepare}=require('../tools/prepare-sywp.cjs'),sywp=require('../tools/sywp.cjs');
const root=path.resolve(__dirname,'../..'),bin=process.env.FFMPEG_BIN||path.join(root,'build/ffmpeg-9.0.1-essentials_build/bin'),ffmpeg=path.join(bin,process.platform==='win32'?'ffmpeg.exe':'ffmpeg');
const fixtureFfmpeg=process.env.BELLEWALL_TEST_FFMPEG||ffmpeg;
function clean(dir){assert.equal(path.dirname(path.resolve(dir)),path.resolve(os.tmpdir()));assert.match(path.basename(dir),/^bellewall-product-/);fs.rmSync(dir,{recursive:true,force:true});}
test('real conversion distinguishes cover crop, contain padding and stretch pixels',{skip:!fs.existsSync(fixtureFfmpeg)},()=>{
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'bellewall-product-'));
 try{
  const pixels=Buffer.alloc(16*8*3);for(let y=0;y<8;y++)for(let x=0;x<16;x++)pixels[(y*16+x)*3+(x<8?0:2)]=255;
  fs.writeFileSync(path.join(dir,'source.ppm'),Buffer.concat([Buffer.from('P6\n16 8\n255\n'),pixels]));
  const source=path.join(dir,'source.mkv'),r=cp.spawnSync(fixtureFfmpeg,['-nostdin','-v','error','-loop','1','-i',path.join(dir,'source.ppm'),'-t','0.4','-r','10','-c:v','ffv1',source],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
  const make=(name,extra)=>{const file=path.join(dir,name+'.sywp');prepare(source,file,{width:8,height:16,frames:2,fps:10,...extra});return sywp.read(file);};
  const left=make('left',{fit:'cover',x:0}),right=make('right',{fit:'cover',x:1}),contain=make('contain',{fit:'contain'}),stretch=make('stretch',{fit:'stretch'}),mp4=make('compressed',{encoding:'mp4'});
  const at=(p,x,y)=>p.payload.readUInt16LE((y*8+x)*2);
  assert.ok((at(left,4,8)&0xf800)>0xf000);assert.ok((at(right,4,8)&31)>28);
  assert.equal(at(contain,4,0),0);assert.notEqual(at(contain,4,8),0);assert.notEqual(at(stretch,4,0),0);
  assert.equal(contain.manifest.display.fit,'contain');assert.equal(stretch.manifest.frames,2);
  assert.equal(mp4.manifest.container,'mp4');assert.equal(mp4.manifest.codec,'mpeg4-part2');assert.deepEqual(mp4.manifest.requiredFeatures,['video-mp4v-v1']);assert.equal(mp4.manifest.frames,2);
  const allKey=make('all-keyframes',{encoding:'mp4',gop:1});
  const syncFrames=p=>{const at=p.indexOf(Buffer.from('stss'));return at<0?2:p.readUInt32BE(at+8);};
  assert.equal(syncFrames(allKey.payload),2);assert.equal(syncFrames(mp4.payload),1);
  assert.throws(()=>make('invalid-gop',{encoding:'mp4',gop:0}),/Keyframe interval/);
  assert.throws(()=>sywp.encode(mp4.manifest,Buffer.from(mp4.payload).fill(0,4,8)),/MP4/);
 }finally{clean(dir);}
});
function packageBytes(entry,data){const items=[['project.json',Buffer.from(JSON.stringify({file:entry,type:'scene'}))],[entry,data]],parts=[];const u32=n=>{const b=Buffer.alloc(4);b.writeUInt32LE(n);return b;},str=s=>{const b=Buffer.from(s);return Buffer.concat([u32(b.length),b]);};parts.push(str('PKGM0014'),u32(items.length));let offset=0;for(const [name,b]of items){parts.push(str(name),u32(offset),u32(b.length));offset+=b.length;}return Buffer.concat([...parts,...items.map(i=>i[1])]);}
test('WebUI previews actual MPKG video entry, preserves web title, cleans successful and failed jobs',async()=>{
 const {start}=require('../tools/sywp-webui.cjs'),dir=fs.mkdtempSync(path.join(os.tmpdir(),'bellewall-product-')),server=start(0,dir);await new Promise(r=>server.once('listening',r));const url='http://127.0.0.1:'+server.address().port;
 try{
  const page=await(await fetch(url)).text(),token=page.match(/'X-BelleWall-Token':'([0-9a-f]+)'/)[1],headers={'X-BelleWall-Token':token};
  const video=Buffer.alloc(24);video.writeUInt32BE(24);video.write('ftyp',4);video.write('isom',8);
  let r=await fetch(url+'/preview?ext=mpkg',{method:'POST',headers,body:packageBytes('real.mp4',video)});assert.equal(r.status,200);assert.deepEqual(Buffer.from(await r.arrayBuffer()),video);
  r=await fetch(url+'/preview?ext=mpkg',{method:'POST',headers,body:packageBytes('scene.json',Buffer.from('{}'))});assert.equal(r.status,400);await r.text();
  r=await fetch(url+'/convert?ext=html&options='+encodeURIComponent(JSON.stringify({title:'测试时钟'})),{method:'POST',headers,body:fs.readFileSync(path.join(root,'prototype/content/clock.html'))});assert.equal(r.status,200);assert.equal(sywp.decode(Buffer.from(await r.arrayBuffer())).manifest.title,'测试时钟');
  for(let i=0;i<20&&fs.readdirSync(dir).length;i++)await new Promise(r=>setTimeout(r,20));assert.deepEqual(fs.readdirSync(dir),[]);
 }finally{await new Promise(r=>server.close(r));clean(dir);}
});
