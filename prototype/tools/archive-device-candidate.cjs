'use strict';
// Archive the exact installed combination, without rebuilding or changing old checkpoints.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..');
const out=path.resolve(process.argv[2]||path.join(root,'dist/device-candidate-20260920-r10'));
const evidence=path.join(root,'research/evidence/device/candidate-20260920');
const session=JSON.parse(fs.readFileSync(path.join(evidence,'session.json'),'utf8').replace(/^\uFEFF/,''));
for(const kind of ['video-600','web-600']){
 const last=session.attempts.filter(a=>a.content===kind).at(-1);
 if(!last||!last.result.includes('passed')||!last.userObserved)throw Error('Missing completed visual acceptance: '+kind);
}
if(fs.existsSync(out))throw Error('Archive already exists; choose a new destination');
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const walk=p=>fs.readdirSync(p,{withFileTypes:true}).flatMap(e=>e.isDirectory()?walk(path.join(p,e.name)):[path.join(p,e.name)]);
const groups=[
 {role:'native',revision:'r10',files:['bellepaper.exe','bellepaper-selfsigned.sisx']},
 {role:'renderer',revision:'r3',files:['bellerenderhost.exe','bellerendercandidate.dll','bellerendercandidate.rsc','bellerender-selfsigned.sisx']},
 {role:'qt-content',revision:'r4',files:['bellewall.exe','bellewall.rsc','bellewall_reg.rsc','bellewall-selfsigned.sisx','bellecontent-selfsigned.sisx','content/case-0.sywp','content/clock.sywp']}
];
const plan=[];
for(const g of groups){
 const base=path.join(root,'dist/daily-candidate-20260920-'+g.revision);
 const sums=new Map(fs.readFileSync(path.join(base,'SHA256SUMS.txt'),'utf8').trim().split(/\r?\n/).map(l=>[l.slice(66),l.slice(0,64)]));
 const selected=[...g.files,...walk(path.join(base,'source')).map(p=>path.relative(base,p).replaceAll('\\','/')),'validation.json'];
 for(const name of selected){
   const source=path.join(base,name),expected=sums.get(name);
   if(!expected||sha(source)!==expected)throw Error('Source checkpoint hash mismatch: '+source);
   const dest=name.startsWith('source/')?'provenance/'+g.role+'/'+name:name==='validation.json'?'provenance/'+g.role+'/validation.json':name;
   plan.push({source,dest,expected});
 }
}
for(const source of walk(evidence))plan.push({source,dest:'evidence/'+path.relative(evidence,source).replaceAll('\\','/'),expected:sha(source)});
for(const name of ['CANDIDATE-20260920.md','SYWP-FORMAT.md','tools/analyze-device-session.cjs','tools/archive-device-candidate.cjs','tests/device-session.test.cjs']){
 const source=path.join(root,'prototype',name);plan.push({source,dest:'review/'+name,expected:sha(source)});
}
fs.mkdirSync(out,{recursive:true});
for(const p of plan){const dest=path.join(out,p.dest);fs.mkdirSync(path.dirname(dest),{recursive:true});fs.copyFileSync(p.source,dest);if(sha(dest)!==p.expected)throw Error('Copy mismatch: '+p.dest);}
const files=groups.flatMap(g=>g.files.map(name=>({name,sha256:sha(path.join(out,name)),sourceRevision:g.revision,role:g.role})));
fs.writeFileSync(path.join(out,'validation.json'),JSON.stringify({date:'2026-09-20',purpose:'Exact device-tested installed combination; not a new rebuild',device:session.device,files,deviceEvidence:'evidence/session.json',sourceProvenance:groups.map(g=>({role:g.role,revision:g.revision,directory:'provenance/'+g.role+'/source'})),limitations:session.limitations},null,2)+'\n');
fs.writeFileSync(path.join(out,'README.md'),'# BelleWall 分阶段实机候选\n\n该目录保存本轮实机实际安装的精确组合：native r10、renderer r3、Qt 与内容 r4。各自原始构建源码与验证清单位于 provenance；没有用重新构建的不同哈希文件替代已测包。\n\n验收范围、已知短暂黑闪、退出恢复偶发超时及测量限制见 [候选报告](review/CANDIDATE-20260920.md) 与 [实机记录](evidence/session.json)。只覆盖 Nokia 603 / 113.010.1506 / Qt 4.8.1 / 四页默认黑色。不是无限常驻或跨固件兼容声明。\n\n程序和内容 SISX 位于本目录根部；安装前先停止并完成恢复，不覆盖未处理的恢复记录。旧已验收回退 dist/background-dirty-pages-20260919 未修改。SYWP 规范和 PC 工具见对应源码。完整文件校验见 SHA256SUMS.txt。\n');
const all=walk(out).sort();fs.writeFileSync(path.join(out,'SHA256SUMS.txt'),all.map(p=>sha(p)+'  '+path.relative(out,p).replaceAll('\\','/')).join('\n')+'\n');
console.log('Verified device archive: '+out+' ('+all.length+' files)');
