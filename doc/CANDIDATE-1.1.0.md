# BelleWall 1.1.0 测试候选 / Test candidate

这是待 E7、603 实机验收的测试版本。E6 的尺寸计算已做离线验证，尚未验证 E6 固件及实机。正式版仍为 1.0.2。

## 安装与文件

先在旧版停止壁纸并确认桌面恢复。解压注入器测试 ZIP，只安装根目录的 `BelleWall-1.1.0-video-candidate.sisx`，其中已包含同版本的程序、播放器和注入器。`injector/bellerender-selfsigned.sisx` 仅供单独调试，正常安装无需重复安装。ZIP 没有额外外层文件夹。

制作工具 ZIP 完整解压后运行根目录的 `Start-BelleWall.cmd`；需要 Node.js，Windows x64 FFmpeg 已随包提供。语言可在右上角切换，手机主界面也有“语言 / Language”。

## 本轮改动

- MP4 优先尝试系统解码器的向前逐帧移动，核对定位结果；不支持或结果不准确时回退到定位取帧。优先请求 RGB565，必要时回退 RGB888，复用位图包装对象。
- 制作工具可选择 RGB565 或 MP4V SYWP；MP4 关键帧间隔可选 1、5、10、30 帧，默认 10。间隔越小通常越利于定位，但文件可能更大。未宣称硬件加速或已达到 30 fps。
- 导入复用 1 MiB 缓冲区，读取、SHA-256 和写入在同一遍完成，进度最多约每 250 ms 更新；保留各阶段耗时日志。与 1.0.3 相比新增缓冲区复用，实际提速幅度须实测。
- 旋转或桌面布局重建时停止发布旧尺寸的帧，稳定后重新取得 FBS 缓存，按新尺寸继续。尺寸相同但缓存重建也会失效旧帧；原桌面恢复记录保留。
- 移除播放路径中固定 360×640 的要求，支持 cover、contain、stretch 和方向限制。候选画布边长 2–2048、最多 1,048,576 像素，视频／网页画布宽度须为偶数；解码器另有设备限制。
- 旧网页按声明的画布尺寸绘制并缩放；定义 `bellewallResize(width,height)` 的网页接收当前桌面视口。暂停期间网页可释放并重建，活动时间继续。
- 手机及制作工具支持中文、英语；手机默认跟随系统语言，其他语言回退英语。系统文件选择器等系统控件仍由系统语言决定。

## 建议测试顺序

1. 两台手机分别导入 `samples/` 中的 20／30 fps RGB565 和 MP4，优先选择 E 盘或可用的 F 盘。先用“60 秒检查”，再持续播放。
2. E7 连续执行竖屏→横屏→竖屏至少十次；在横屏停止，确认原生桌面恢复。再试锁屏／解锁、切到应用再返回、多桌面切换、暂停后旋转及恢复。
3. `portrait-only` 样本应在横屏暂停，返回竖屏继续；其他样本跟随屏幕。`contain` 应完整显示并补边，`stretch` 应填满目标。640×480 样本可在 E7／603 验证缩放，但不能代替 E6 实机测试。
4. `test-web-responsive.sywp` 显示视口尺寸和活动秒数，旋转后尺寸应变化；`test-web-clock.sywp` 验证旧网页没有缩成左上角的小块。
5. 比较普通 MP4（GOP 10）与 `mp4-gop1` 的流畅度及循环，保留 RGB565 作为参照。用原来的约 130 MB 文件比较导入时间，分别记录来源盘、目标盘、文件大小、耗时。
6. 手机中英切换后测试壁纸管理、导入、预览、停止、诊断；制作工具切换语言后确认尺寸、编码、名称等选项不变。
7. 每轮导出诊断：`SYWP import profile` 有读盘、哈希、写盘、UI 与刷新耗时；`VIDEO profile` 有定位与取帧等待耗时；`candidate-metrics.csv` 有读帧和绘制耗时。遇到失败请保留错误码与日志。

离线校验见 `validation.json`，ZIP 内的文件校验值见 `SHA256SUMS.txt`。本轮不包含直接导入裸 MP4；请用制作工具封装为 MP4 SYWP。实机帧率、MP4 控制器能力及旋转恢复结果仍待验证。

## English quick start

Stop the old wallpaper and confirm desktop restoration before upgrading. Install only `BelleWall-1.1.0-video-candidate.sisx` at the archive root; it contains the matching app, native player and injector. The separate injector is for debugging. Import the supplied SYWP samples to C, E or F, then start with the 60-second check.

This candidate adds display resizing and recovery, cover/contain/stretch fitting, orientation restrictions, Chinese/English UI, MP4 frame-stepping and RGB565 extraction with fallbacks, configurable MP4 keyframe intervals, and import-buffer reuse. Hardware acceleration and sustained 30 fps have not been verified.

Test repeated portrait/landscape transitions, locking, app switching, multiple desktop pages and stopping in landscape. Check the portrait-only sample, both web samples and both MP4 keyframe intervals. The responsive web sample should report the new viewport size after rotation. Compare import time using the original large RGB565 package and export diagnostics afterwards. E6 geometry is covered offline; E6 firmware and device compatibility remain unverified. Bare MP4 import is not included; export an MP4 SYWP from the workshop.

For the PC workshop, extract the full tools archive and run `Start-BelleWall.cmd`. Node.js is required; Windows x64 FFmpeg is included. Both interfaces have a language selector. System-owned dialogs use the operating system language.
