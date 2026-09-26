'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict');
const {analyze}=require('../tools/analyze-longrun.cjs');
const h=(wall,active,frames,heap=100000,cpu=0,error=0)=>`HEALTH wall_us=${wall} active_us=${active} heap_bytes=${heap} cells=30 thread_cpu_us=${cpu} cpu_error=${error} state=3 frames=${frames}\n`;
test('24-hour sparse health data measures cumulative frames, not sample rows',()=>{
 let log='CANDIDATE state = 1\n';for(let m=0;m<=1440;m++)log+=h(m*60000000,m*30000000,m*300,100000,m*1000000);
 const r=analyze(log);assert.equal(r.observedWallSeconds,86400);assert.equal(r.observedActiveSeconds,43200);assert.equal(r.contentFramesPerActiveSecond,10);assert.equal(r.privateHeapBytes.netChange,0);assert.equal(r.healthGaps.length,0);
});
test('locked interval does not lower active playback rate',()=>{const r=analyze(h(0,0,0)+h(60000000,0,0)+h(120000000,60000000,600));assert.equal(r.contentFramesPerActiveSecond,10);});
test('unsigned frame counter wrap stays positive',()=>{const r=analyze(h(0,0,0xfffffff0)+h(10000000,10000000,84));assert.equal(r.contentFrames,100);assert.equal(r.frameCounterWraps,1);});
test('rotation without session start is disclosed; missing samples remain visible',()=>{const r=analyze(h(60000000,60000000,600)+h(240000000,240000000,2400));assert.equal(r.sessionStartPresent,false);assert.equal(r.healthGaps[0].gapSeconds,180);});
test('uses last session and retains failed recovery and CPU uncertainty',()=>{const r=analyze('CANDIDATE state = 1\n'+h(0,0,0)+h(1000000,1000000,10)+'CANDIDATE state = 1\n'+h(0,0,0)+h(60000000,60000000,500,200000,0,-5)+'CANDIDATE recovery result = -33\n');assert.equal(r.contentFrames,500);assert.equal(r.nativeMainThreadCpuPercent,null);assert.equal(r.restorationComplete,false);assert.equal(r.failures[0].code,-33);});
test('rejects backward clocks and impossible active time',()=>{assert.throws(()=>analyze(h(100,100,1)+h(90,90,2)));assert.throws(()=>analyze(h(0,0,0)+h(100,200,1)));});
