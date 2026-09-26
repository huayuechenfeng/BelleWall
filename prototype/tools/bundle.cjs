'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process');
const root=path.resolve(__dirname,'../..'),target=path.join(root,'dist/bellewall-prototype-0.1.zip');
const files=['README.md','prototype','research/evidence/prototype','research/scripts/inspect-mpkg.cjs',...['bellewall.exe','bellewall.rsc','bellewall_reg.rsc','bellewall.pkg','bellewall-selfsigned.sisx','bellewall-unsigned.sis','sample.mp4','animation.html','config.ini','SHA256SUMS.txt','signature-info.txt'].map(n=>'dist/'+n)];
files.push('research/evidence/device/fp2-analysis',...['bellepaper.exe','bellepaper-selfsigned.sisx','belleprobe.exe','belleprobe-selfsigned.sisx','THIRD-PARTY.txt'].map(n=>'dist/'+n));
// Include readable device findings, not the verbose base64 SIS upload transcript.
for(const name of fs.readdirSync(path.join(root,'research/evidence/device'))) {
 if(name.endsWith('.md')||name.endsWith('.log')||name.endsWith('.json')||(name.startsWith('benchmark-')&&name.endsWith('.csv'))||['direct-probe-restart.txt','final-processes.txt','final-native-processes.txt'].includes(name))files.push('research/evidence/device/'+name);
}
const r=cp.spawnSync('tar.exe',['-a','-cf',target,'-C',root,...files],{encoding:'utf8'});if(r.error||r.status)throw Error(r.error||r.stderr);console.log(target+' ('+fs.statSync(target).size+' bytes)');
