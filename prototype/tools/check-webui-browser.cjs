'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert/strict');
const {start}=require('./sywp-webui.cjs');
const root=path.resolve(__dirname,'../..'),out=path.join(root,'build/product-browser-qa');fs.mkdirSync(out,{recursive:true});
const browser=process.env.BELLE_BROWSER||'C:/Program Files/Google/Chrome/Application/chrome.exe';
async function run(){
 const server=start(0,path.join(out,'jobs'));await new Promise(r=>server.once('listening',r));
 const profile=fs.mkdtempSync(path.join(out,'profile-')),chrome=cp.spawn(browser,['--headless=new','--disable-gpu','--no-first-run','--no-default-browser-check','--remote-debugging-port=0','--user-data-dir='+profile,'about:blank'],{windowsHide:true});let socket;
 try{
  const endpoint=await new Promise((resolve,reject)=>{let stderr='';const timeout=setTimeout(()=>reject(Error('Browser startup timed out')),15000);chrome.stderr.on('data',b=>{stderr+=b;const m=stderr.match(/DevTools listening on (ws:\/\/[^\s]+)/);if(m){clearTimeout(timeout);resolve(m[1]);}});chrome.on('error',reject);});
  const list=await(await fetch('http://'+new URL(endpoint).host+'/json/list')).json();socket=new WebSocket(list.find(t=>t.type==='page').webSocketDebuggerUrl);await new Promise((r,j)=>{socket.addEventListener('open',r,{once:true});socket.addEventListener('error',j,{once:true});});
  let id=0,conversionId;const pending=new Map();socket.addEventListener('message',e=>{const m=JSON.parse(e.data);if(m.method==='Network.responseReceived'&&m.params.response.url.includes('/convert?'))conversionId=m.params.requestId;const p=pending.get(m.id);if(p){pending.delete(m.id);clearTimeout(p.timer);m.error?p.reject(Error(JSON.stringify(m.error))):p.resolve(m.result);}});
  const call=(method,params={})=>new Promise((resolve,reject)=>{const n=++id,timer=setTimeout(()=>{pending.delete(n);reject(Error('CDP timeout: '+method));},20000);pending.set(n,{resolve,reject,timer});socket.send(JSON.stringify({id:n,method,params}));});
  const evaluate=async expression=>{const r=await call('Runtime.evaluate',{expression,awaitPromise:true,returnByValue:true});if(r.exceptionDetails)throw Error(JSON.stringify(r.exceptionDetails));return r.result.value;};
  const wait=async expression=>{for(let i=0;i<160;i++){if(await evaluate(expression))return;await new Promise(r=>setTimeout(r,100));}throw Error('UI condition timed out: '+expression);};
  await call('Page.enable');await call('Network.enable');await call('Emulation.setDeviceMetricsOverride',{width:1100,height:900,deviceScaleFactor:1,mobile:false});await call('Page.navigate',{url:'http://127.0.0.1:'+server.address().port});await wait('document.readyState==="complete"&&!!document.querySelector("#make")');
  const choose=async file=>{const {root:doc}=await call('DOM.getDocument');const {nodeId}=await call('DOM.querySelector',{nodeId:doc.nodeId,selector:'#file'});await call('DOM.setFileInputFiles',{nodeId,files:[file]});};
  const mpkg=path.join(root,'build/product-assets/preview.mpkg');if(fs.existsSync(mpkg)){await choose(mpkg);await wait('document.querySelector("#video").readyState>=1&&!document.querySelector("#make").disabled');assert.equal(await evaluate('document.querySelector("#status").textContent'),'');}
  await evaluate('document.querySelector("#fit").value="contain";document.querySelector("#fit").dispatchEvent(new Event("input"))');assert.equal(await evaluate('document.querySelector("#video").style.objectFit'),'contain');
  fs.writeFileSync(path.join(out,'desktop.png'),Buffer.from((await call('Page.captureScreenshot',{format:'png',captureBeyondViewport:true})).data,'base64'));
  await choose(path.join(root,'prototype/content/clock.html'));await evaluate('window.__originalFetch=window.fetch;window.fetch=async function(input,init){const r=await window.__originalFetch(input,init);if(String(input).startsWith("/convert?"))window.__qaBytes=Array.from(new Uint8Array(await r.clone().arrayBuffer()));return r;};document.querySelector("#title").value="浏览器验收时钟";document.querySelector("#make").click()');await wait('!!document.querySelector("#status a")');
  const manifest=require('./sywp.cjs').decode(Buffer.from(await evaluate('window.__qaBytes'))).manifest;assert.equal(manifest.title,'浏览器验收时钟');assert.equal(manifest.kind,'web');
  await call('Emulation.setDeviceMetricsOverride',{width:390,height:844,deviceScaleFactor:1,mobile:true});assert.equal(await evaluate('document.documentElement.scrollWidth<=window.innerWidth'),true);
  fs.writeFileSync(path.join(out,'mobile.png'),Buffer.from((await call('Page.captureScreenshot',{format:'png',captureBeyondViewport:true})).data,'base64'));
  fs.writeFileSync(path.join(out,'result.json'),JSON.stringify({mpkgPreview:fs.existsSync(mpkg),containPreview:true,webDownload:manifest,mobileNoHorizontalOverflow:true},null,2));console.log('Browser QA passed: '+out);
 }finally{if(socket)socket.close();chrome.kill();await new Promise(r=>chrome.exitCode!==null?r():chrome.once('exit',r));await new Promise(r=>server.close(r));}
}
run().catch(e=>{console.error(e);process.exitCode=1;});
