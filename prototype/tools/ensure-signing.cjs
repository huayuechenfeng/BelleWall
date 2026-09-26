'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process');
module.exports=function(root){
 const dir=path.join(root,'build/signing');fs.mkdirSync(dir,{recursive:true});
 const cert=path.join(dir,'prototype.cer'),key=path.join(dir,'prototype.key');
 if(fs.existsSync(cert)&&fs.existsSync(key))return;
 if(fs.existsSync(cert)||fs.existsSync(key))throw Error('Incomplete local signing pair; preserve and inspect build/signing before regenerating');
 const r=cp.spawnSync(process.env.OPENSSL||'C:/Program Files/Git/mingw64/bin/openssl.exe',['req','-new','-newkey','rsa:2048','-nodes','-x509','-sha1','-days','3650','-subj','/CN=BelleWall Development Only/O=Local Prototype','-keyout',key,'-out',cert],{encoding:'utf8'});
 if(r.error||r.status)throw Error(r.error||r.stderr);
};
