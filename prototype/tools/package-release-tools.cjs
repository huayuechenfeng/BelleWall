'use strict';
// Archive the prepared PC tools at ZIP root, with no "./" or version wrapper.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..');
const source=path.resolve(process.argv[2]||path.join(root,'build/release-1.0.2/tools'));
const output=path.resolve(process.argv[3]||path.join(root,'dist/release/1.0.2/BelleWall-1.0.2-tools.zip'));
if(fs.existsSync(output))throw Error('Tools ZIP exists; choose a new output');
const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const manifest=fs.readFileSync(path.join(source,'SHA256SUMS.txt'),'utf8').trim().split(/\r?\n/).map(line=>{const match=line.match(/^([0-9a-f]{64})  (.+)$/);if(!match)throw Error('Invalid tools hash line');return {hash:match[1],name:match[2]};});
for(const item of manifest)if(sha(path.join(source,item.name))!==item.hash)throw Error('Staging hash mismatch: '+item.name);
for(const required of ['Start-BelleWall.cmd','prototype/tools/sywp-webui.cjs','ffmpeg/bin/ffmpeg.exe'])if(!manifest.some(item=>item.name===required))throw Error('Required tools file missing: '+required);
const run=args=>{const result=cp.spawnSync('tar.exe',args,{encoding:'utf8',windowsHide:true,maxBuffer:4*1024*1024});if(result.error||result.status)throw Error(String(result.error||result.stderr||result.stdout));return result.stdout;};
run(['-a','-cf',output,'-C',source,...fs.readdirSync(source).sort()]);
const entries=run(['-tf',output]).trim().split(/\r?\n/).map(x=>x.replaceAll('\\','/'));
if(entries.some(x=>x==='.'||x.startsWith('./')||x.startsWith('BelleWall-1.0.2-tools/')))throw Error('Tools ZIP has an extra wrapper directory');
const listed=new Set(entries);
for(const item of manifest)if(!listed.has(item.name))throw Error('Tools ZIP entry missing: '+item.name);
if(!listed.has('SHA256SUMS.txt'))throw Error('Tools ZIP hash list missing');
const build=fs.realpathSync(path.join(root,'build'));
const verify=fs.mkdtempSync(path.join(build,'tools-zip-verify-'));
try{
 run(['-xf',output,'-C',verify]);
 for(const item of manifest)if(sha(path.join(verify,item.name))!==item.hash)throw Error('Extracted tools hash mismatch: '+item.name);
 if(sha(path.join(verify,'SHA256SUMS.txt'))!==sha(path.join(source,'SHA256SUMS.txt')))throw Error('Extracted tools hash list mismatch');
}finally{
 const resolved=fs.realpathSync(verify);
 if(path.dirname(resolved)!==build||!path.basename(resolved).startsWith('tools-zip-verify-'))throw Error('Unsafe tools verification directory');
 fs.rmSync(resolved,{recursive:true});
}
console.log(JSON.stringify({output,sha256:sha(output),verifiedFiles:manifest.length,rootFiles:fs.readdirSync(source).length}));
