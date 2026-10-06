'use strict';
// Run render-demo-wallpapers.py first. Outputs stay outside the source tree.
const fs = require('fs'), path = require('path'), crypto = require('crypto');
const sywp = require('./sywp.cjs');
const root = path.resolve(__dirname, '../..');
const rendered = path.join(root, 'build/wallpaper-pack-v1/rendered');
const out = path.join(root, 'build/wallpaper-pack-v1/package');
const sha = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
const titles = {
  'neon-city': ['Neon-City', '霓虹城市 / Neon City'],
  'moonlit-mountains': ['Moonlit-Mountains', '月夜群山 / Moonlit Mountains'],
  'pixel-planet': ['Pixel-Planet', '像素星球 / Pixel Planet'],
};
fs.mkdirSync(out, {recursive: true});
const report = JSON.parse(fs.readFileSync(path.join(rendered, 'render-validation.json'))).map(item => {
  const payload = fs.readFileSync(path.join(rendered, item.name + '.rgb565'));
  if (sha(payload) !== item.rawSha256) throw Error('Rendered frames changed: ' + item.name);
  const [suffix, title] = titles[item.name];
  const manifest = {format: 'sywp', version: 1, title, kind: 'video', width: item.width,
    height: item.height, loop: true, pause: 'resume', pixelFormat: 'rgb565le', stride: item.width * 2,
    fpsNumerator: item.fps, fpsDenominator: 1, frames: item.frames,
    display: {orientation: 'auto', fit: 'contain', rotation: 'follow-display', background: '#000000'}};
  const name = 'BelleWall-' + suffix + '.sywp';
  const bytes = sywp.encode(manifest, payload);
  fs.writeFileSync(path.join(out, name), bytes, {flag: 'wx'});
  const decoded = sywp.read(path.join(out, name));
  if (sha(decoded.payload) !== item.rawSha256 || JSON.stringify(decoded.manifest) !== JSON.stringify(manifest))
    throw Error('SYWP round-trip mismatch: ' + name);
  return {...item, file: name, bytes: bytes.length, sha256: sha(bytes),
    previewSha256: sha(fs.readFileSync(path.join(root, 'assets/wallpapers/previews', item.name + '.gif'))),
    packageValidated: true, deviceTested: false};
});
fs.writeFileSync(path.join(out, 'wallpapers-validation.json'), JSON.stringify(report, null, 2) + '\n');
console.log(JSON.stringify(report, null, 2));
