'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),path=require('path'),os=require('os'),crypto=require('crypto');
const sywp=require('../tools/sywp.cjs');
const base={format:'sywp',version:1,title:'时钟与动画',kind:'video',width:180,height:320,loop:true,pause:'resume',pixelFormat:'rgb565le',stride:360,fpsNumerator:10,fpsDenominator:1,frames:2,display:sywp.display};
test('roundtrip arbitrary screen ratios, horizontal canvases, and 10/20/30 fps',()=>{
 for(const [width,height] of [[180,320],[360,640],[640,360],[640,480],[480,640],[240,240]])for(const fps of [10,20,30]){
  const m={...base,width,height,stride:width*2,fpsNumerator:fps},p=Buffer.alloc(width*height*4,0x85),b=sywp.encode(m,p),r=sywp.decode(b);assert.deepEqual(r.manifest,m);assert.deepEqual(r.payload,p);
  const stream=sywp.frameStream(m,p);assert.equal(stream.readUInt32LE(8),width);assert.equal(stream.readUInt32LE(24),fps);assert.equal(stream.readUInt32LE(44),p.length);
 }
});
test('fractional frame rate and future unknown required features',()=>{const p=Buffer.alloc(230400);assert.doesNotThrow(()=>sywp.encode({...base,fpsNumerator:30000,fpsDenominator:1001},p));assert.throws(()=>sywp.encode({...base,requiredFeatures:['future-codec']},p));});
test('corruption/truncation/trailing bytes/version/flags rejected before payload use',()=>{const good=sywp.encode(base,Buffer.alloc(230400));for(const at of [0,4,8,12,16,20,24,56,64,good.length-1]){const bad=Buffer.from(good);bad[at]^=0x80;assert.throws(()=>sywp.decode(bad));}for(const n of [0,16,63,64,good.length-1])assert.throws(()=>sywp.decode(good.subarray(0,n)));assert.throws(()=>sywp.decode(Buffer.concat([good,Buffer.from([0])])));});
test('semantic limits and extent mismatch cannot be hidden by valid digest',()=>{for(const change of [{width:181},{height:0},{frames:0},{fpsNumerator:61},{fpsDenominator:0},{stride:1},{display:{...sywp.display,fit:'unknown'}}])assert.throws(()=>sywp.encode({...base,...change},Buffer.alloc(230400)));assert.throws(()=>sywp.encode(base,Buffer.alloc(115200)));});
test('self-contained clock HTML roundtrip and no invalid UTF-8',()=>{const m={...base,kind:'web',entry:'index.html'},p=fs.readFileSync(path.join(__dirname,'../content/clock.html'));assert.deepEqual(sywp.decode(sywp.encode(m,p)).payload,p);assert.throws(()=>sywp.encode(m,Buffer.from([255])));});
test('loop boundaries map to exact frame extents',()=>{const m={...base,frames:3},p=Buffer.concat([Buffer.alloc(115200,1),Buffer.alloc(115200,2),Buffer.alloc(115200,3)]),stream=sywp.frameStream(m,p);for(const index of [0,1,2,3,29,30,6000])assert.equal(stream[48+(index%3)*115200],index%3+1);});
test('HTTP UI enforces local origin token, rejects invalid uploads, and produces valid web package',async()=>{
 const {start}=require('../tools/sywp-webui.cjs'),dir=fs.mkdtempSync(path.join(os.tmpdir(),'bellewall-http-test-')),server=start(0,dir);await new Promise(r=>server.once('listening',r));const url='http://127.0.0.1:'+server.address().port;
 try{const html=await (await fetch(url)).text();assert.match(html,/30 fps/);const token=html.match(/'X-BelleWall-Token':'([a-f0-9]+)'/)[1];assert.equal((await fetch(url+'/convert',{method:'POST',body:'x'})).status,403);assert.equal((await fetch(url+'/convert?ext=swf',{method:'POST',headers:{'X-BelleWall-Token':token},body:'x'})).status,400);
 const response=await fetch(url+'/convert?ext=html',{method:'POST',headers:{'X-BelleWall-Token':token},body:fs.readFileSync(path.join(__dirname,'../content/clock.html'))});assert.equal(response.status,200);assert.equal(sywp.decode(Buffer.from(await response.arrayBuffer())).manifest.kind,'web');
 }finally{await new Promise(r=>server.close(r));fs.rmSync(dir,{recursive:true,force:true});}
});
