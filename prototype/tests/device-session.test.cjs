'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict');
const {analyze}=require('../tools/analyze-device-session.cjs');
const header='wall_us,active_us,source,work_us,consumer\n';
const log='CANDIDATE state = 1\nHEALTH wall_us=0 active_us=0 heap_bytes=100 cells=2 thread_cpu_us=0 cpu_error=0 state=3\nHEALTH wall_us=10000000 active_us=1000000 heap_bytes=100 cells=2 thread_cpu_us=500000 cpu_error=0 state=3\nCANDIDATE stopped; recovery transaction complete\n';
test('excludes a paused interval from foreground timing without hiding it',()=>{
 const r=analyze(header+'100000,100000,298,1000,1\n200000,200000,299,2000,2\n5300000,300000,0,3000,3\n5400000,400000,1,1000,4',log);
 assert.equal(r.uninterruptedIntervalP95Us,100000);assert.equal(r.pauseGaps[0].pausedUs,5000000);assert.equal(r.contentSamplesPerActiveSecond,10);assert.equal(r.observedSourceWraps,1);assert.equal(r.sampledMainThreadCpuPercent,5);assert.equal(r.restorationComplete,true);
});
test('uses only the last appended native session and retains restore failure evidence',()=>{
 const r=analyze(header+'0,0,0,100,0\n100000,100000,1,100,1',log+'CANDIDATE state = 1\nCANDIDATE recovery result = -33\n','web');
 assert.equal(r.healthSamples,0);assert.equal(r.restorationComplete,false);assert.deepEqual(r.restorationErrors,[-33]);assert.equal(r.observedSourceWraps,null);
});
test('rejects backwards clocks and malformed metrics',()=>{
 assert.throws(()=>analyze(header+'200000,200000,0,1,0\n100000,100000,1,1,1',log));
 assert.throws(()=>analyze(header+'0,1,0,1,0',log));
 assert.throws(()=>analyze(header+'x,0,0,1,0',log));
});
