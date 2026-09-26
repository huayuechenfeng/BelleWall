# 进阶教程：自己编写网页类 SYWP

网页类 SYWP 直接保存一个 HTML 文件，由手机旧版 QtWebKit 实时绘制，并非把网页预渲染成图片集。适合时钟、日历、简单几何动画；视频请走视频制作流程。

## 作者必须遵守的约定

1. 单个 UTF-8 HTML，不超过 256 KiB，包含 HTML、CSS 和 JavaScript；当前画布固定 180×320。
2. 定义全局 `function bellewallStep(activeMs)`。宿主活动时调用它，每次据 activeMs 更新一帧；不要自行启动 setInterval、setTimeout、requestAnimationFrame、音视频或 CSS 自动动画。
3. activeMs 是有效播放时间，暂停时不增长。动画的位置应由它计算，避免依赖“每帧加一”或页面上次状态。
4. 现实日期时间每帧用 `new Date()` 获取，解锁后才能立即校准。不要用 activeMs 当作实时时钟。
5. 使用 ES5 风格语法：var、普通 function；避免 let、const、箭头函数、模板字符串、Promise、模块、现代 CSS 特效。桌面浏览器正常不表示旧 QtWebKit 正常。
6. 不依赖网络、外部 JS／CSS、任意本地文件、插件、Java、弹窗或跳转。宿主禁止这些能力。优先用内联 CSS、DOM 或 canvas；内嵌图片应在手机验证，不能依赖浏览器缓存。

锁屏或离开桌面时，宿主会释放网页；恢复时重新加载，JavaScript 变量和 DOM 重置，但 activeMs 保留。不要依赖本地存储或把页面当作一直驻留的应用。随机效果应从 activeMs 和固定种子推导，才能恢复一致。

## 一个完整例子

将下面内容保存为 `my-wallpaper.html`（UTF-8）：

```html
<!doctype html>
<html><head><meta charset="utf-8"><style>
html,body { margin:0; width:180px; height:320px; overflow:hidden; background:#101820; }
canvas { display:block; }
</style></head><body>
<canvas id="wall" width="180" height="320"></canvas>
<script>
var canvas = document.getElementById('wall');
var ctx = canvas.getContext('2d');
function pad(n) { return n < 10 ? '0' + n : '' + n; }
function bellewallStep(activeMs) {
  var now = new Date();
  ctx.fillStyle = '#101820';
  ctx.fillRect(0, 0, 180, 320);
  ctx.fillStyle = '#e4f3ed';
  ctx.font = '24px sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText(pad(now.getHours()) + ':' + pad(now.getMinutes()) + ':' +
               pad(now.getSeconds()), 90, 130);
  var x = 90 + 55 * Math.sin(activeMs / 1200);
  ctx.fillStyle = '#9de8c6';
  ctx.beginPath(); ctx.arc(x, 195, 10, 0, Math.PI * 2, false); ctx.fill();
}
bellewallStep(0);
</script></body></html>
```

同一示例见 [starter.html](../prototype/content/starter.html)，现有实时时钟见 [clock.html](../prototype/content/clock.html)。它们不自动启动计时器，直接在浏览器打开时只显示静止一帧。

## 在电脑上调试

用浏览器打开 HTML，在开发者控制台手动执行 `bellewallStep(1000)`、`bellewallStep(2000)`，确认位置变化。检查 `typeof bellewallStep` 为 `function`，控制台没有异常。可在控制台临时创建调试计时器，但不要把它写进提交给手机的 HTML。

多次调用同一 activeMs 应得到相同动画状态（现实时间文字除外）。刷新页面再调用同一数值，可模拟暂停后重建。避免每帧创建大量 DOM、加载字体或分配大数组；保持单帧工作量小。

## 打包为 SYWP

最简单的方法是在制作工具中选择 HTML，填写标题、180×320 和竖屏／自动方向，生成并下载。网页模式不需要 FFmpeg，视频帧率／帧数／速度不适用于网页。

也可在工具包根目录运行：

```powershell
node prototype/tools/sywp.cjs web my-wallpaper.html my-wallpaper.sywp
node prototype/tools/sywp.cjs inspect my-wallpaper.sywp
```

CLI 默认标题是 WebKit wallpaper。需要自定义标题时使用 WebUI，或调用打包库，保存以下脚本为工具包根目录 `pack-my-web.cjs`：

```javascript
const fs = require('fs');
const sywp = require('./prototype/tools/sywp.cjs');
const manifest = {
  format: 'sywp', version: 1, title: '我的时钟', kind: 'web',
  width: 180, height: 320, loop: true, pause: 'resume', entry: 'index.html',
  display: {
    orientation: 'portrait', fit: 'cover',
    rotation: 'follow-display', background: '#000000'
  }
};
fs.writeFileSync('my-clock.sywp',
  sywp.encode(manifest, fs.readFileSync('my-wallpaper.html')), {flag: 'wx'});
```

执行 `node pack-my-web.cjs`。输出已存在时先改名，防止覆盖作品。脚本的现代 Node 语法只在电脑运行；手机 HTML 仍应使用 ES5。

不要用 ZIP 改后缀冒充 SYWP。工具会生成固定 64 字节文件头、UTF-8 JSON manifest、HTML payload，以及覆盖 manifest＋payload 的 SHA-256。修改任何内容后都应重新打包；手工只改标题或 HTML 会导致校验失败。完整字段与字节布局见 [格式规范](SYWP-FORMAT.md)。

## 手机上验证

复制 SYWP → 导入 → 预览 → 60 秒检查。确认四页内容在图标下面、滑页正常、结束恢复正常。再检查一次锁屏后重建：现实时间校准，动画根据 activeMs 接续。网页运行性能取决于内容复杂度，不能只凭浏览器速度判断。

加载失败时检查编码、文件大小、函数名称、脚本语法和异常；打包器的结构校验不会完整执行你的脚本。当前导入器只接受 180×320 网页；不要通过改 manifest 声称已支持 E6、横屏或响应式视口。未来格式支持更多画布，但当前播放器尚未实现方向切换。
