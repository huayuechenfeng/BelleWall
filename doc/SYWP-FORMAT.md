# SYWP 1 壁纸包规范（容器版本 1）

统一扩展名为 `.sywp`，MIME 为 `application/vnd.bellewall.sywp`；`symp` 不作为另一种格式。SYWP 是内容交换格式，不绑定某个机型或屏幕尺寸。现有正式版支持无压缩 RGB565 视频帧集和手机实时 WebKit 自包含页面；1.0.3 测试候选另支持下述 MP4V 载荷。SWF、实时 scene MPKG 不在本版范围。

**版本边界：** 1.0.2 及更早的手机程序只接受无压缩 RGB565 视频。MP4V 配置是 1.0.3 测试候选；E7、603 的系统解码和持续帧率尚未验收，格式支持范围会根据实测调整。直接导入裸 `.mp4` 尚未实现，见 [下一阶段计划](NEXT-PHASE-PLAN.md)。

## 容器

所有整数为无符号小端。文件严格由 64 字节头、UTF-8 JSON manifest、一个连续 payload 组成；禁止尾随数据。不使用 ZIP，不提取任意包内路径，没有压缩炸弹或目录穿越入口。

| 偏移 | 长度 | 内容 |
|---|---|---|
| 0 | 4 | 精确字节 `53 59 57 50`（SYWP） |
| 4 | 4 | major version = 1 |
| 8 | 4 | headerBytes = 64 |
| 12 | 4 | manifestBytes，1–16384 |
| 16 | 4 | payloadBytes，最大 268435456（256 MiB） |
| 20 | 4 | flags = 0；未知值拒绝 |
| 24 | 32 | SHA-256(manifest 原始字节 + payload 原始字节) |
| 56 | 8 | 保留，全零 |

总长度必须等于 `64 + manifestBytes + payloadBytes`。SHA-256 检测损坏，不是作者身份签名。禁止把 JSON 直接作为脚本执行。未知 major、未知 requiredFeatures、未知 kind / 像素格式必须拒绝；未使用的普通元数据字段可以忽略。扩展多画布、多资源、音频或新编码需要新增明确的 required feature 或 major，不能悄悄重解释旧字段。

## 通用 manifest

```json
{
  "format": "sywp",
  "version": 1,
  "title": "My wallpaper",
  "kind": "video",
  "width": 360,
  "height": 640,
  "loop": true,
  "pause": "resume",
  "display": {
    "orientation": "auto",
    "fit": "cover",
    "rotation": "follow-display",
    "background": "#000000"
  },
  "requiredFeatures": [],
  "pixelFormat": "rgb565le",
  "stride": 720,
  "fpsNumerator": 30,
  "fpsDenominator": 1,
  "frames": 300
}
```

- `title`：非空字符串，最长 120 个 UTF-16 code units。
- `width`：偶数 2–2048；`height`：1–2048。这是内容画布，不是手机型号。支持 180×320、360×640、640×360、640×480、方屏等比例；不据此宣称 E6 已适配。
- `loop: true`、`pause: "resume"`：停止有效播放时钟后从原位置继续。现实日期时间独立读取系统时间。
- `orientation`：`auto` 跟随显示方向；`portrait` / `landscape` 限定可播放方向。不支持目标方向时应暂停或拒绝，不能忽略。
- `fit`：`cover` 等比铺满、居中裁剪；`contain` 等比完整显示、以 background 补边；`stretch` 独立缩放宽高。以当前显示坐标系计算，不假定手机永远是竖屏。
- `rotation: "follow-display"`：显示坐标随系统方向改变，重新计算适配；不把预烘焙视频内容任意旋转 90°。画布本身横向或纵向的含义由 width/height 决定。
- 视频固定源画布；网页可响应视口。未来运行时旋转后应更新视口并可调用 `bellewallResize(width,height)`，保留有效播放时间；无法完成此契约的运行时必须声明限制。

## 视频 payload

RGB565 配置中，帧按时间顺序排列。`RGB565LE`：R5 高位、G6 中位、B5 低位；逐像素小端；行自上至下，不含 alpha。`stride = width * 2`；偶数宽度保证四字节行对齐。`payloadBytes = frames * stride * height`，乘法以至少 64 位检查。帧数至少 1，且受总包大小约束。

`fpsNumerator / fpsDenominator` 在 1–60 fps；分子 1–60000，分母 1–1001。10/20/30 fps 都是常规选项，也可表达 30000/1001。帧时刻为 `frameIndex * denominator / numerator`。循环周期为 `frames * denominator / numerator`。以有效单调时间选择目标帧，落后时跳帧；解码／显示队列不得积压。PC 调整速度会重新采样输出，不再给手机叠加第二个速度参数。

导入后的 BWV2 是内部手机帧流，不是另一个对外包格式。它的 48 字节头为 12 个 LE uint32：`0x32565742,48,width,height,stride,1,fpsNumerator,fpsDenominator,frames,frameBytes,48,payloadBytes`，后接原始帧。`1` 表示 RGB565LE。手机按目标位置读单帧，不把全视频载入内存。

### 1.0.3 测试候选：MP4V 载荷

压缩配置使用相同的 SYWP v1 头、长度上限和 SHA-256 校验。manifest 的 `kind` 仍为 `video`，但用以下字段代替 RGB565 的 `pixelFormat` 和 `stride`：

```json
{
  "container": "mp4",
  "codec": "mpeg4-part2",
  "requiredFeatures": ["video-mp4v-v1"],
  "fpsNumerator": 30,
  "fpsDenominator": 1,
  "frames": 300
}
```

payload 是一份完整 MP4 文件，包含一个恒定帧率的 `mp4v` 视频轨、`ftyp`、`moov` 和非空 `mdat`；不包含音轨、字幕或附加视频轨。制作工具采用 MPEG-4 Part 2、YUV420P、无 B 帧、按帧率设定关键帧间隔，并把 `moov` 放在媒体数据前。PC 校验器核对视频轨实际宽高、样本数和 `stts` 帧率与 manifest 一致；手机导入时验证包 SHA-256 和基本 `ftyp` 结构，播放时交给系统解码器。手机将 payload 原样保存为内部 `.mp4`，不转成 BWV2；旧版读取器因未知 `requiredFeatures` 拒绝此配置。手机候选当前仅接受 180×320／360×640、10／20／30 fps、竖屏；解码性能需实机验证。

## WebKit payload 与作者契约

`kind: "web"`，`entry: "index.html"`，payload 是一个不超过 256 KiB 的 UTF-8 HTML。不需 video 专属字段。资源内嵌 HTML/CSS/data URL；不提供任意本地文件、远程网络、插件或 Java 权限。

实现 ES5 风格 `function bellewallStep(activeMs) { ... }`。宿主在活动期间调用，使用 activeMs 驱动动画；现实时间每次通过 `new Date()` 获取。页面不要自行启动 setInterval、requestAnimationFrame、媒体播放或 CSS 动画。暂停时当前候选销毁页面，恢复时重建；DOM、JS 变量和未声明的随机状态会重置，宿主 activeMs 保留。需要持久效果应由 activeMs 确定性计算。不能把现代浏览器可打开当作 QtWebKit 兼容证据。

参考：[clock.html](../prototype/content/clock.html)。打包：

```powershell
node prototype/tools/sywp.cjs web prototype/content/clock.html build/my-clock.sywp
node prototype/tools/sywp.cjs inspect build/my-clock.sywp
```

## 制作工具与当前设备能力

```powershell
node prototype/tools/sywp-webui.cjs
# 浏览器打开 http://127.0.0.1:8765，选择源文件、画布、帧率、裁剪位置、张数、速度。
node prototype/tools/prepare-sywp.cjs input.mp4 build/my-video.sywp '{"width":360,"height":640,"fps":30,"frames":300,"speed":1,"x":0.5,"y":0.5,"encoding":"rgb565"}'
node prototype/tools/prepare-sywp.cjs input.mp4 build/my-video-mp4.sywp '{"width":360,"height":640,"fps":30,"frames":300,"encoding":"mp4"}'
```

WebUI 仅绑定本机；POST 校验随机令牌与 Host。视频／MPKG 转换不执行包内脚本。视频型 MPKG 根据实际入口识别，即使 project.type 为 scene 也可转换；实时 scene 和 SWF 拒绝。当前制作工具将 fit 同时应用于源视频到包画布：cover 按位置裁剪、contain 居中补背景色、stretch 拉伸；这些结果已烘焙为 payload 像素。manifest 的 `display.fit` 仍定义画布到手机屏幕的适配策略，不能要求播放器再次裁剪原始素材。输出超过存储上限时拒绝。WebUI 临时任务保存在 build/sywp-webui，并在处理与传输结束后清理；异常终止遗留目录可在工具关闭后清理。

当前手机库保留原始 manifest 作为同名 `.json` 元数据，显示作者提供的标题；统一选择指针 `selected-wallpaper.txt` 在校验与落盘成功后原子更新。这些属于手机内部实现，不是 SYWP 容器字段。失败导入保持此前选择，已有未恢复会话时拒绝修改。

1.0.0／1.0.1 手机播放器导入支持 180×320 实时网页，以及 180×320 或 360×640 视频；视频供帧约 10 fps。1.0.2 视频版按 BWV2 的分子／分母帧率调度，显示请求最高 30 fps，落后时跳帧；高于 30 fps 的 RGB565 素材可以导入但按 30 fps 上限取样。1.0.3 测试候选增加 MP4V 系统解码路径，实际帧率未验证。网页仍按约 10 fps 生成。手机暂不支持横屏／其他比例播放，方向或背景对象不匹配时停止；其他包可由 PC 制作、检查，手机明确拒绝。E6 和旋转切换仍未适配。
