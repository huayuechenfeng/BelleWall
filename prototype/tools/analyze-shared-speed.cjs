'use strict';
const fs = require('fs'), path = require('path');
const input = process.argv[2];
if (!input) throw Error('Usage: node analyze-shared-speed.cjs input.csv [output.json] [--local-background]');
const dirtyRectangles = process.argv.includes('--dirty-rectangles');
const localBackground = dirtyRectangles || process.argv.includes('--local-background');
const lines = fs.readFileSync(input, 'utf8').trim().split(/\r?\n/);
const header = lines.shift().split(',');
const rows = lines.map(line => Object.fromEntries(line.split(',').map((v, i) => [header[i], Number(v)])));
if (!rows.length || rows.some(r => header.some(k => !Number.isFinite(r[k])))) throw Error('Invalid timing rows');
const mean = (a, k) => a.reduce((s, r) => s + r[k], 0) / a.length;
const phases = [...new Set(rows.map(r => r.phase))].map(phase => {
  const a = rows.filter(r => r.phase === phase), last = a[a.length - 1];
  return {phase, frames: a.length, targetUs: a[0].target_us,
    completedCyclesPerSecond: a.length * 1e6 / last.elapsed_us,
    meanUs: Object.fromEntries(['gate_us','paint_us','redraw_us','sample_us','cycle_us','clear_us','sync_us'].filter(k => header.includes(k)).map(k => [k, mean(a, k)]))};
});
const samples = rows.map((r, serial) => ({...r, serial})).filter(r => r.sample_us > 0 && r.marker_count > 0);
const lagFits = [0, 1, 2, 3].map(lag => ({lag,
  meanAbsolutePixelError: samples.length ? samples.reduce((s,r) => s + Math.abs(r.marker_x - (((r.serial-lag)*13)%296+31.5)),0)/samples.length : null
}));
const changed = samples.slice(1).filter((r,i) => r.marker_x !== samples[i].marker_x).length;
const byPhase = [...new Set(samples.map(r => r.phase))].map(phase => {
  const a = samples.filter(r => r.phase === phase);
  return {phase, samples:a.length, successivePositionChanges:a.slice(1).filter((r,i) => r.marker_x !== a[i].marker_x).length};
});
const report = {input: path.basename(input), redrawMode: dirtyRectangles ? 'desktop-background-DrawNow-dirty-rectangles' : localBackground ? 'desktop-background-DrawDeferred' : 'global-ClearAllRedrawStores', phases, screen: {samples:samples.length, successivePositionChanges:changed, byPhase, lagFits},
  limitations: ['Rates are completed host cycles, not physical LCD presentation timestamps.',
    'Screen sampling adds work and can return an earlier composed frame.',
    'Lag fits compare marker centroid with the known 64-pixel-wide test marker; they are not display latency measurements.',
    localBackground ? 'Producer cycle timings exclude asynchronous desktop background rendering; mode must be corroborated by the native and plugin logs.' : 'Global ClearAllRedrawStores remains diagnostic, not a targeted production redraw path.']};
if (process.argv[3] && !process.argv[3].startsWith('--')) fs.writeFileSync(process.argv[3], JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify(report,null,2));
