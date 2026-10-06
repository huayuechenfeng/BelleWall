# BelleWall 1.1.2：暂时关闭 MP4 壁纸

## 判断依据

2026-10-06，CODA 确认 E7 的程序、原生播放器和注入器均为 1.1.1。日志显示刷新握手通过，系统控制器 `101f8514 / Real Video Player` 打开 360×640、30 fps、60 帧 MP4，进入第一帧处理后没有获得任何输出帧，随后守护超时终止进程。`VIDEO profile frames=0 seek_frames=1` 出现在退出前，未完成正常播放器退出。这已不同于 1.1.0 的 panic 46。

这些证据说明当前 BelleWall 的系统取帧及退出路径不可靠，并不证明所有 Belle 手机都不能解码 MP4。要可靠支持，仍需专门处理控制器的取帧、异步事件和退出行为，并重新验证暂停、循环和恢复；超出本轮小修复范围。按用户要求，本版暂时关闭产品中的 MP4 壁纸功能，保留实现及规范供后续独立研究。

## 本版行为

- 手机拒绝新 MP4 SYWP 导入、已有 MP4 壁纸的启动及预览，显示 -7111 和转换为 RGB565 的说明。
- 已导入的 MP4 文件不会自动删除，仍可在壁纸管理中删除。原有 RGB565、网页和库位置选择保持可用。
- PC 工具移除 MP4 SYWP 导出选项及关键帧参数，命令行和 HTTP 转换入口同样拒绝 MP4 输出。
- **MP4 作为源视频仍可导入制作工具并转换为 RGB565 SYWP**。视频型 MPKG 预览和转换继续保留。
- 横竖屏重建、通用尺寸映射、中英界面和 1.1.1 刷新握手保留。
- 保留 SYWP MP4 载荷规范与检查能力，但不宣称当前手机产品支持其播放。

## 安装与检查

先解锁手机并回到桌面，在 BelleWall 中完成“恢复桌面”。本次失败日志记录恢复时桌面不可用、恢复记录保留，因此确认恢复完成后再升级。

安装 injector-test.zip 根目录的 `BelleWall-1.1.2-video-candidate.sisx`，它包含全部配套组件；独立 injector 文件夹仅供调试。升级后完整重启手机。制作工具使用新的 1.1.2 tools-test.zip，完整解压后运行 `Start-BelleWall.cmd`，需要 Node.js，FFmpeg 随包提供。两个 ZIP 都没有额外外层文件夹。

手机样本仅包含 RGB565 与网页：20／30 fps、横屏 contain、640×480 stretch、仅竖屏，以及旧网页和响应式网页，共七个。先测原来正常的 RGB565 和网页，确认启动、旋转、停止恢复；再确认旧 MP4 会被明确拒绝，且可以删除。电脑上用 MP4 源视频制作 RGB565 包并导入手机。

E7 横屏和双语已有用户正面反馈；响应式网页可识别视口。非标准屏没有实机验证，不把 E7 的尺寸检测等同于 E6 全机兼容。本版仍为测试候选，正式版仍为 1.0.2。

## English

MP4 wallpaper support is temporarily disabled after the E7 1.1.1 test opened the media successfully but produced no frames and failed to exit normally. This is a limitation of the current BelleWall integration, not proof that Belle phones cannot decode MP4.

MP4 SYWP import, launch, preview and export are disabled. Existing MP4 library files remain deletable. MP4 source videos and video-based MPKG can still be converted to RGB565 SYWP on the PC. RGB565, web wallpapers, rotation and bilingual UI remain available.

Restore the desktop before upgrading, install only the combined root SISX, and fully restart the phone. Use the matching 1.1.2 workshop, which requires Node.js and includes FFmpeg. Seven RGB565/web samples are supplied. Device acceptance of this candidate is pending; E6 has not been tested on a physical device.
