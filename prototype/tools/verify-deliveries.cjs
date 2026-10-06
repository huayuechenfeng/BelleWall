'use strict';
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),dist=path.join(root,'dist');
const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
let checked=0;
for(const channel of ['release','test']){
 const base=path.join(dist,channel);if(!fs.existsSync(base))continue;
 for(const version of fs.readdirSync(base)){
  const dir=path.join(base,version);if(!fs.statSync(dir).isDirectory())throw Error('Unexpected file in '+base);
  const manifest=JSON.parse(fs.readFileSync(path.join(dir,'CHANNEL.json'),'utf8'));
  if(manifest.channel!==channel||manifest.version!==version||!/^\d+\.\d+\.\d+$/.test(version))throw Error('Channel/version mismatch: '+dir);
  const listed=new Set(manifest.artifacts.map(x=>x.name));
  const files=fs.readdirSync(dir).filter(x=>x!=='CHANNEL.json');
  if(files.length!==listed.size||files.some(x=>!listed.has(x)))throw Error('Unlisted delivery file: '+dir);
  for(const item of manifest.artifacts){
   if(item.name!==path.basename(item.name)||sha(path.join(dir,item.name))!==item.sha256)throw Error('Delivery hash mismatch: '+path.join(dir,item.name));
   if(channel==='release'&&/(candidate|test)/i.test(item.name))throw Error('Test artifact in release channel: '+item.name);
   if(channel==='test'&&/\.sisx$|\.zip$/i.test(item.name)&&!/(candidate|test)/i.test(item.name))throw Error('Unmarked candidate artifact: '+item.name);
   checked++;
  }
  if(channel==='release'){
   for(const required of [`BelleWall-${version}.sisx`,`BelleWall-${version}-tools.zip`,'SHA256SUMS.txt','RELEASE-NOTES.md','validation.json'])if(!listed.has(required))throw Error('Release file missing: '+path.join(dir,required));
   const sums=fs.readFileSync(path.join(dir,'SHA256SUMS.txt'),'utf8').trim().split(/\r?\n/);
   for(const line of sums){const m=line.match(/^([0-9a-f]{64})  (.+)$/);if(!m||!listed.has(m[2])||sha(path.join(dir,m[2]))!==m[1])throw Error('SHA256SUMS mismatch: '+line);}
   if(sums.length!==files.length-1)throw Error('Incomplete SHA256SUMS: '+dir);
  }
 }
}
console.log(JSON.stringify({status:'passed',files:checked,channels:['release','test']}));
