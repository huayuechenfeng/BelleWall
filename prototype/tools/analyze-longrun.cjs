'use strict';
// Continuous sessions use cumulative health counters, never CSV row counts.
const fs=require('fs');
function analyze(text){
 text=text.replace(/\r\n/g,'\n');
 const start=text.lastIndexOf('CANDIDATE state = 1\n'),log=start>=0?text.slice(start):text;
 const rows=[...log.matchAll(/HEALTH wall_us=(\d+) active_us=(\d+) heap_bytes=(\d+) cells=(\d+) thread_cpu_us=(\d+) cpu_error=(-?\d+) state=(\d+) frames=(\d+)/g)].map(m=>m.slice(1).map(Number));
 if(rows.length<2)throw Error('At least two cumulative health samples required');
 if(rows.some(r=>r.some(n=>!Number.isSafeInteger(n))||r[1]>r[0]||r[7]>0xffffffff))throw Error('Invalid health sample');
 let frames=0,wraps=0;const gaps=[];
 for(let i=1;i<rows.length;i++){const a=rows[i-1],b=rows[i];if(b[0]<=a[0]||b[1]<a[1]||b[1]-a[1]>b[0]-a[0]+20000||b[4]<a[4])throw Error('Non-monotonic session counters');if(b[7]<a[7])wraps++;frames+=(b[7]-a[7]+0x100000000)%0x100000000;if(b[0]-a[0]>90000000)gaps.push({afterSample:i,gapSeconds:(b[0]-a[0])/1e6});}
 const first=rows[0],last=rows.at(-1),wall=(last[0]-first[0])/1e6,active=(last[1]-first[1])/1e6,heap=rows.map(r=>r[2]);
 return {samples:rows.length,sessionStartPresent:start>=0,observedWallSeconds:wall,observedActiveSeconds:active,contentFrames:frames,contentFramesPerActiveSecond:active?frames/active:null,frameCounterWraps:wraps,healthGaps:gaps,privateHeapBytes:{first:heap[0],last:heap.at(-1),min:Math.min(...heap),max:Math.max(...heap),netChange:heap.at(-1)-heap[0]},nativeMainThreadCpuPercent:rows.every(r=>r[5]===0)?100*(last[4]-first[4])/(last[0]-first[0]):null,restorationComplete:log.includes('CANDIDATE stopped; recovery transaction complete'),failures:[...log.matchAll(/CANDIDATE (playback|recovery) result = (-\d+)/g)].map(m=>({stage:m[1],code:Number(m[2])})),limitations:['Observed health-sample window excludes unsampled start/end intervals.','Frame deltas assume no more than one 32-bit wrap between samples.','Heap and CPU cover only the native process/main thread, not WebKit, desktop, FBS or battery.','A report is evidence, not automatic release approval; user observation and final process/journal checks are separate.']};
}
if(require.main===module){try{const files=process.argv.slice(2);if(!files.length)throw Error('Pass native log files in chronological order (previous, current)');console.log(JSON.stringify(analyze(files.map(p=>fs.readFileSync(p,'utf8').replace(/\r\n/g,'\n')).join('\n')),null,2));}catch(e){console.error(e.message);process.exitCode=1;}}
module.exports={analyze};
