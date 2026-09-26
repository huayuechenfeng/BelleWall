'use strict';
// Distinguish active playback, pauses, content submissions and sampled main-thread CPU.
const fs=require('fs');
function quantile(values,p){const a=[...values].sort((x,y)=>x-y);return a.length?a[Math.ceil(a.length*p)-1]:null;}
function analyze(csv,nativeLog,kind='video'){
  const lines=csv.trim().split(/\r?\n/);
  if(lines.shift()!=='wall_us,active_us,source,work_us,consumer')throw Error('Unknown CSV header');
  const rows=lines.map(line=>line.split(',').map(Number));
  if(!rows.length||rows.some(r=>r.length!==5||r.some(v=>!Number.isSafeInteger(v)||v<0)||r[1]>r[0]))throw Error('Invalid metrics');
  const intervals=[],activeIntervals=[],pauseGaps=[];let wraps=0;
  for(let i=1;i<rows.length;i++){
    const wall=rows[i][0]-rows[i-1][0],active=rows[i][1]-rows[i-1][1];
    if(wall<0||active<0||active>wall+20000)throw Error('Non-monotonic or inconsistent clock');
    intervals.push(wall);
    if(wall-active>20000)pauseGaps.push({afterRow:i,wallUs:wall,activeUs:active,pausedUs:wall-active});
    else activeIntervals.push(wall);
    if(kind==='video'&&rows[i][2]<rows[i-1][2])wraps++;
  }
  const start=nativeLog.lastIndexOf('CANDIDATE state = 1');
  if(start<0)throw Error('Missing session start in native log');
  const log=nativeLog.slice(start),health=[...log.matchAll(/HEALTH wall_us=(\d+) active_us=(\d+) heap_bytes=(\d+) cells=(\d+) thread_cpu_us=(\d+) cpu_error=(-?\d+) state=(\d+)/g)].map(m=>m.slice(1).map(Number));
  const cpuSamples=health.filter(h=>h[5]===0),firstCpu=cpuSamples[0],lastCpu=cpuSamples.at(-1);
  const activeSpan=rows.at(-1)[1]-rows[0][1];
  return {
    kind,contentSamples:rows.length,wallSeconds:rows.at(-1)[0]/1e6,activeSeconds:rows.at(-1)[1]/1e6,
    contentSamplesPerActiveSecond:activeSpan>0?(rows.length-1)*1e6/activeSpan:null,
    uninterruptedIntervalP95Us:quantile(activeIntervals,.95),uninterruptedIntervalMaxUs:quantile(activeIntervals,1),
    workP95Us:quantile(rows.map(r=>r[3]),.95),workMaxUs:quantile(rows.map(r=>r[3]),1),
    pauseGaps,observedSourceWraps:kind==='video'?wraps:null,
    healthSamples:health.length,
    privateHeapBytes:health.length?{min:Math.min(...health.map(h=>h[2])),max:Math.max(...health.map(h=>h[2])),first:health[0][2],last:health.at(-1)[2]}:null,
    sampledMainThreadCpuPercent:firstCpu&&lastCpu[0]>firstCpu[0]?100*(lastCpu[4]-firstCpu[4])/(lastCpu[0]-firstCpu[0]):null,
    cpuSampleWindowSeconds:firstCpu&&lastCpu?(lastCpu[0]-firstCpu[0])/1e6:0,
    restorationComplete:log.includes('CANDIDATE stopped; recovery transaction complete'),
    restorationErrors:[...log.matchAll(/CANDIDATE recovery result = (-\d+)/g)].map(m=>Number(m[1])),
    limitations:['Content samples are not LCD-presented frames.','CPU covers only the native main thread, averaged over the sampled wall-clock window including pauses.','Private heap excludes Qt, desktop and FBS allocations.','Pauses are inferred from wall-minus-active clock gaps; resume latency is not independently measured.']
  };
}
if(require.main===module){try{console.log(JSON.stringify(analyze(fs.readFileSync(process.argv[2],'utf8'),fs.readFileSync(process.argv[3],'utf8'),process.argv[4]||'video'),null,2));}catch(e){console.error(e.message);process.exitCode=1;}}
module.exports={analyze};
