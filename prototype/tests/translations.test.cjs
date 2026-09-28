'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),path=require('path');
const root=path.resolve(__dirname,'../..');
test('phone user-visible Chinese strings have English translations with matching placeholders',()=>{
 const messages=JSON.parse(fs.readFileSync(path.join(root,'prototype/translations/phone-en.json'),'utf8'));
 for(const file of ['sywpimport.h','preparationui.h','maintenanceui.h']){
  const source=fs.readFileSync(path.join(root,'prototype/src',file),'utf8');
  for(const match of source.matchAll(/"((?:[^"\\]|\\.)*)"/g)){const value=JSON.parse(match[0]);if(!/[\u4e00-\u9fff]/.test(value))continue;assert.ok(messages[value],file+': missing '+value);assert.deepEqual((messages[value].match(/%\d+/g)||[]).sort(),(value.match(/%\d+/g)||[]).sort(),value);}
 }
});
