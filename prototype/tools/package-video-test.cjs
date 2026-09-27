'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..');
const source=path.resolve(process.argv[2]||path.join(root,'build/checkpoints/BelleWall-1.0.2-video-candidate-v2'));
const output=path.resolve(process.argv[3]||path.join(root,'dist/test/1.0.2/BelleWall-1.0.2-injector-test.zip'));
if(fs.existsSync(output))throw Error('Bundle output exists; choose a new clean output');
const build=fs.realpathSync(path.join(root,'build'));
const staging=fs.mkdtempSync(path.join(build,'video-test-bundle-'));
try{
const bundle=staging;
const items=[
 ['BelleWall-1.0.2-video-candidate.sisx','BelleWall-1.0.2-video-candidate.sisx'],
 ['bellerender-selfsigned.sisx','injector/bellerender-selfsigned.sisx'],
 ['test-20fps-360x640.sywp','samples/test-20fps-360x640.sywp'],
 ['test-30fps-360x640.sywp','samples/test-30fps-360x640.sywp'],
 ['validation.json','validation.json']
];
const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const validation=JSON.parse(fs.readFileSync(path.join(source,'validation.json'),'utf8'));
if(validation.version!=='1.0.2'||validation.sha256!==sha(path.join(source,items[0][0]))||validation.components.find(x=>x.name===items[1][0])?.sha256!==sha(path.join(source,items[1][0])))throw Error('Installer or injector does not match validated candidate');
for(const sample of validation.samples){if(sample.sha256!==sha(path.join(source,sample.name)))throw Error('Sample does not match validated candidate: '+sample.name);}
for(const [name,relative] of items){
 const target=path.join(bundle,relative);fs.mkdirSync(path.dirname(target),{recursive:true});fs.copyFileSync(path.join(source,name),target);
 if(sha(target)!==sha(path.join(source,name)))throw Error('Copy mismatch: '+name);
}
const readme=`# BelleWall 1.0.2 E7／603 视频与注入器测试包

正常测试只安装 ZIP 根目录的 BelleWall-1.0.2-video-candidate.sisx。它已内含同版本桌面注入器、原生播放进程和 BelleWall 程序。injector/bellerender-selfsigned.sisx 是同版本注入器的独立组件，供检查或单独调试；正常测试不要重复安装。

先在旧版停止动态壁纸并完成桌面恢复。安装组合包后，把 samples 中的两份 SYWP 复制到手机，在 BelleWall 中导入并选择 C、E 或 F 存储盘；F 盘仅在设备可用时出现。两份样本分别为 360×640、20 fps／30 fps、2 秒循环。两台手机各自先用“60 秒检查”，观察实际流畅度、暂停恢复和停止后的桌面恢复。

30 fps 是播放与注入器的请求上限，仍需 E7、603 实机验收。文件哈希在 SHA256SUMS.txt；构建及核验信息在 validation.json。原始 SYWP 不随导入操作删除。
`;
fs.writeFileSync(path.join(bundle,'README.md'),readme);
const hashes=[...items.map(([,relative])=>relative),'README.md'].map(relative=>sha(path.join(bundle,relative))+'  '+relative).join('\n')+'\n';
fs.writeFileSync(path.join(bundle,'SHA256SUMS.txt'),hashes);
const run=args=>{const result=cp.spawnSync('tar.exe',args,{encoding:'utf8',windowsHide:true,maxBuffer:1024*1024});if(result.error||result.status)throw Error(String(result.error||result.stderr||result.stdout));return result.stdout;};
run(['-a','-cf',output,'-C',staging,...fs.readdirSync(staging).sort()]);
const listed=new Set(run(['-tf',output]).trim().split(/\r?\n/).map(x=>x.replaceAll('\\','/')));
for(const relative of [...items.map(([,name])=>name),'README.md','SHA256SUMS.txt'])if(!listed.has(relative))throw Error('ZIP entry missing: '+relative);
if([...listed].some(name=>name.startsWith('./')||name.startsWith('BelleWall-1.0.2-test/')))throw Error('Unexpected ZIP wrapper directory');
console.log(JSON.stringify({output,bytes:fs.statSync(output).size,sha256:sha(output),fileCount:items.length+2},null,2));
}finally{
 const resolved=fs.realpathSync(staging);
 if(path.dirname(resolved)!==build||!path.basename(resolved).startsWith('video-test-bundle-'))throw Error('Unsafe bundle staging path');
 fs.rmSync(resolved,{recursive:true});
}
