'use strict';
// Read-only verification of the three paired product packages. Extraction uses
// an isolated temporary directory; only that freshly created directory is removed.
const fs=require('fs'),path=require('path'),os=require('os'),cp=require('child_process'),crypto=require('crypto');
const version=process.env.BELLEWALL_PRODUCT_VERSION||'1.0.0';
if(!/^\d+\.\d+\.\d+$/.test(version))throw Error('Invalid product version');
const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const profiles=[
 {sis:'bellerender-selfsigned.sisx',uid:0xe7b31106,dependency:null,files:['bellerendercandidate.dll','bellerendercandidate.rsc','bellerenderhost.exe','bellerenderlongrun.dll','bellerenderlongrun.rsc'],uids:{0:0xe7b31126,2:0xe7b31108,3:0xe7b31136}},
 {sis:'bellepaper-selfsigned.sisx',uid:0xe7b31103,dependency:0xe7b31106,files:['bellepaper.exe'],uids:{0:0xe7b31103}},
 {sis:'bellewall-selfsigned.sisx',uid:0xe7b31101,dependency:0xe7b31103,files:['bellewall.exe','bellewall.rsc','bellewall_reg.rsc','config.ini','animation.html','sample.mp4','THIRD-PARTY.txt','belleweb.exe','belleweb.rsc','belleweb_reg.rsc'],uids:{0:0xe7b31101,7:0xe7b31130}}
];
if(Number(version.split('.')[0])>1||Number(version.split('.')[1])>=3)profiles[2].files.push('bellewall.mif');
function destination(name){
 const dir=/\.(exe|dll)$/.test(name)?'sys\\bin':['bellerendercandidate.rsc','bellerenderlongrun.rsc'].includes(name)?'resource\\plugins':/_reg\.rsc$/.test(name)?'private\\10003a3f\\import\\apps':/\.(rsc|mif)$/.test(name)?'resource\\apps':'data\\BelleWall';
 return 'C:\\'+dir+'\\'+name;
}
function manifestMetadata(bytes,profile){
 const text=bytes.toString(bytes[0]===0xff&&bytes[1]===0xfe?'utf16le':'utf8').replace(/^\uFEFF/,'');
 const header=[...text.matchAll(/^#\{[^\r\n]+?\},\s*\(0x([0-9a-f]+)\),\s*(\d+),\s*(\d+),\s*(\d+),\s*TYPE=SA\s*$/gmi)];
 if(header.length!==1||parseInt(header[0][1],16)!==profile.uid||header[0].slice(2).join('.')!==version)throw Error(profile.sis+': package identity/version mismatch');
 const dependencies=[...text.matchAll(/^\(0x([0-9a-f]+)\),\s*(\d+),\s*(\d+),\s*(\d+)\s*,\s*\{[^\r\n]+\}\s*$/gmi)];
 if(dependencies.length!==(profile.dependency===null?0:1)||(profile.dependency!==null&&(parseInt(dependencies[0][1],16)!==profile.dependency||dependencies[0].slice(2).join('.')!==version)))throw Error(profile.sis+': package dependency mismatch');
 const entries=[...text.matchAll(/^"file(\d+)"\s*-\s*"([^"\r\n]+)"([^\r\n]*)$/gm)];
 if(entries.length!==profile.files.length)throw Error(profile.sis+': install entry count mismatch');
 entries.forEach((entry,i)=>{if(Number(entry[1])!==i||entry[2].toLowerCase()!==destination(profile.files[i]).toLowerCase()||!/^,\s*(?:VR,\s*)?FF\s*$/.test(entry[3]))throw Error(profile.sis+': install destination/options mismatch '+profile.files[i]);});
 return {uid:'0x'+profile.uid.toString(16),version,dependency:profile.dependency===null?null:{uid:'0x'+profile.dependency.toString(16),minimumVersion:version}};
}
function verify(directory,{sdk=process.env.BELLE_SDK||'C:/QtSDK/Symbian/SDKs/SymbianSR1Qt474'}={}){
 const base=path.resolve(directory),temporary=fs.mkdtempSync(path.join(os.tmpdir(),'bellewall-sis-check-')),packages=[];
 function run(args){const r=cp.spawnSync(path.join(sdk,'epoc32/tools/dumpsis.exe'),args,{cwd:temporary,encoding:'utf8',windowsHide:true,maxBuffer:8*1024*1024});if(r.error||r.status)throw Error('SIS inspection failed: '+(r.error||r.stderr||r.stdout));return r.stdout;}
 try{
  for(let index=0;index<profiles.length;index++){
   const profile=profiles[index],sis=path.join(base,profile.sis),extracted=path.join(temporary,String(index));fs.mkdirSync(extracted);run(['-x','-d',extracted,sis]);const capabilities=run(['-l',sis]);
   const actualFiles=fs.readdirSync(extracted).filter(n=>/^file\d+$/.test(n));if(actualFiles.length!==profile.files.length)throw Error(profile.sis+': unexpected payload count');
   const manifests=fs.readdirSync(extracted).filter(n=>n.endsWith('.pkg'));if(manifests.length!==1)throw Error(profile.sis+': expected one extracted manifest');
   const metadata=manifestMetadata(fs.readFileSync(path.join(extracted,manifests[0])),profile);
   const payloads=profile.files.map((name,i)=>{
    let original=path.join(base,name);if(!fs.existsSync(original))original=path.join(base,'package-inputs',name);
    const file=path.join(extracted,'file'+i),expected=sha(original),actual=sha(file);if(expected!==actual)throw Error(profile.sis+': payload mismatch '+name);
    const expectedUid=profile.uids[i];if(expectedUid!==undefined){const bytes=fs.readFileSync(file);if(bytes.length<12||bytes.readUInt32LE(8)!==expectedUid)throw Error(profile.sis+': application identity mismatch '+name);if(!capabilities.includes('Executable'+(i+1)+': capabilities matched'))throw Error(profile.sis+': capability mismatch '+name);}
    return {name,destination:destination(name),sha256:actual,...(expectedUid===undefined?{}:{uid3:'0x'+expectedUid.toString(16),capabilitiesMatched:true})};
   });
   packages.push({name:profile.sis,sha256:sha(sis),...metadata,payloads});
  }
  return {status:'all-paired-payloads-identities-destinations-and-dependencies-matched',packages,payloadCount:packages.reduce((n,p)=>n+p.payloads.length,0),limitations:['Static verification does not prove signature trust, installation, privilege availability, first-run registration, or device behavior.']};
 }finally{
  const resolved=fs.realpathSync(temporary),parent=fs.realpathSync(os.tmpdir());if(path.dirname(resolved)!==parent||!path.basename(resolved).startsWith('bellewall-sis-check-'))throw Error('Refusing unsafe temporary cleanup');fs.rmSync(resolved,{recursive:true,force:true});
 }
}
if(require.main===module){try{if(!process.argv[2])throw Error('Usage: node verify-product-packages.cjs <dist-or-checkpoint> [report.json]');const result=verify(process.argv[2]);if(process.argv[3])fs.writeFileSync(path.resolve(process.argv[3]),JSON.stringify(result,null,2));console.log('Verified '+result.packages.length+' packages, '+result.payloadCount+' payloads and all 6 executable/DLL identities.');}catch(e){console.error(e.message);process.exitCode=1;}}
module.exports={verify};
