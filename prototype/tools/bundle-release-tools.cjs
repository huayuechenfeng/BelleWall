'use strict';
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||'build/release-1.0.0/tools');
if(fs.existsSync(out))throw Error('Choose a new tools directory');
function copy(from,to=from){const dst=path.join(out,to);fs.mkdirSync(path.dirname(dst),{recursive:true});fs.copyFileSync(path.join(root,from),dst);}
for(const n of ['README.md','LICENSE'])copy(n);
for(const n of fs.readdirSync(path.join(root,'doc')).filter(n=>n.endsWith('.md')))copy('doc/'+n);
for(const n of fs.readdirSync(path.join(root,'LICENSES')))copy('LICENSES/'+n);
for(const n of ['sywp-webui.cjs','sywp-webui.html','sywp.cjs','prepare-sywp.cjs','mpkg.cjs'])copy('prototype/tools/'+n);
for(const n of ['clock.html','starter.html'])copy('prototype/content/'+n);
for(const kind of ['video','web'])copy('build/product-assets/'+kind+'.sywp','examples/BelleWall-'+kind+'.sywp');
copy('build/product-assets/demo.mp4','examples/demo.mp4');copy('build/product-assets/PROVENANCE.md','examples/PROVENANCE.md');
copy('build/product-assets/preview.mpkg','examples/demo-video.mpkg');
fs.writeFileSync(path.join(out,'Start-BelleWall.cmd'),'@echo off\r\nnode "%~dp0prototype\\tools\\sywp-webui.cjs"\r\nif errorlevel 1 pause\r\n');
const walk=p=>fs.readdirSync(p,{withFileTypes:true}).flatMap(e=>e.isDirectory()?walk(path.join(p,e.name)):[path.join(p,e.name)]);
const files=walk(out).sort();fs.writeFileSync(path.join(out,'SHA256SUMS.txt'),files.map(f=>crypto.createHash('sha256').update(fs.readFileSync(f)).digest('hex')+'  '+path.relative(out,f).replaceAll('\\','/')).join('\n')+'\n');
console.log('Tools bundle: '+out+' ('+files.length+' files)');
