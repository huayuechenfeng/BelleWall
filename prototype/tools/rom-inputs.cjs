'use strict';
// ROM images are external release-validation inputs, never bundled in the repo.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const expected={
 nokia603_fp2:'76d6e4451d657f58a09f8de6313d4aac02c74a42927514b5f266df32730f5816',
 nokia_e7:'f2203cbd0e83f7fdc7e69959be32aeea93a1063c8ef2a2cd42feb6e034900c54',
 nokia603_belle:'ac5c9952c293893a67b06f586b0a4f5f6fecb740ea669fb1d15eaf371fcd445a'
};
function load(config=process.env.BELLEWALL_ROM_CONFIG){
 if(!config)throw Error('Set BELLEWALL_ROM_CONFIG to your ROM path JSON; see prototype/config/rom-inputs.example.json and doc/BUILD.md. Release ROM checks cannot be skipped.');
 const file=path.resolve(config);if(!fs.existsSync(file))throw Error('ROM configuration not found: '+file);
 const values=JSON.parse(fs.readFileSync(file,'utf8')),result={};
 for(const [id,hash] of Object.entries(expected)){
  if(typeof values[id]!=='string'||!values[id].trim())throw Error('Missing ROM path: '+id);
  const image=path.resolve(path.dirname(file),values[id]);
  if(!fs.existsSync(image))throw Error('ROM input not found: '+id+' at '+image);
  if(crypto.createHash('sha256').update(fs.readFileSync(image)).digest('hex')!==hash)throw Error('ROM SHA-256 differs: '+id+'; expected '+hash);
  result[id]=image;
 }
 return result;
}
module.exports={load,expected};
if(require.main===module)console.log(JSON.stringify({status:'verified',inputs:load()},null,2));
