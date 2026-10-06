# BelleWall 1.3 正式版 / Release 1.3

[简体中文](#简体中文) · [English](#english)

## 简体中文

安装包与三个组件统一版本为 **1.3.0**，保持原 UID，可从旧版升级。

### 本次更新

- 新增 Belle 风格图标：薄荷绿与深蓝绿的双层圆角方块，手机菜单与 PC 制作工具统一使用。
- 支持横竖屏切换时暂停、重建画面并恢复播放，以及 cover／contain／stretch 和方向限制。
- 支持非标准画布与响应式网页；单边最多 2048、总像素最多 1,048,576。E6 等非标准屏尚未实机验证。
- 手机与制作工具支持中文／English。
- RGB565 视频最高请求 30 fps，支持 360×640、20／30 fps；导入使用复用缓冲区、复制时校验与节流进度更新。30 fps 是请求上限，不是实测持续帧率；大文件导入优化尚无新的实机耗时记录。
- 壁纸库可选择 C／E／F 盘（以设备实际可用盘符为准）。
- 增加刷新通道握手，检测升级后残留的旧注入器并提示重启。
- 暂停 MP4 压缩壁纸的导入、播放与导出：E7 上可打开视频但无法输出首帧，退出流程也可能失去响应。MP4 等源视频仍可在 PC 转换为 RGB565 SYWP。已有 MP4 壁纸保留供删除。

### 下载与升级

- `BelleWall-1.3.0.sisx`：手机组合安装包，已包含注入器、原生进程与主程序，正常使用只需安装这一个文件。
- `BelleWall-1.3.0-injector.zip`：同一安装包、独立注入器与 RGB565／网页样例，供需要完整手机附件的用户使用；不要重复安装独立注入器。
- `BelleWall-1.3.0-tools.zip`：Windows x64 制作工具，内置 FFmpeg，需自行安装 Node.js。解压后根目录直接运行 `Start-BelleWall.cmd`。

升级前先在旧版“停止并恢复桌面”，确认恢复并退出。安装后**完整重启手机**，再启动壁纸。不要混用不同版本组件。程序安装于 C 盘；壁纸库可放 E／F 盘。Qt 最低 4.7.4，设备须具备相应证书信任与系统权限。

### 验证范围

**2026-10-06：用户已确认 1.3.0 正式版在 Nokia E7 上可用。** 此前的实机反馈也已确认 RGB565 播放、横屏及双语正常；1.3.0 整合 MP4 禁用与新图标。非标准屏仅有尺寸识别／离线覆盖，不能据此认定 E6 已兼容；603 本轮结果待反馈。离线验证已通过：66 项测试、新图标与双语转换流程的浏览器检查、三个组件的 UID／版本／依赖／安装位置／载荷／签名核验，以及 ZIP 内容与文件哈希检查。

## English

The installer and all three phone components are version **1.3.0**. Existing application identifiers are retained for upgrades.

### What's new

- **New Belle-style icon:** two stacked rounded squares in mint and dark teal, used by the phone launcher and PC Workshop.
- **Rotation and image fitting:** pause, rebuild and resume playback when switching between portrait and landscape; choose cover, contain or stretch, and restrict playback to a screen orientation if desired.
- **Non-standard canvases and responsive web wallpapers:** up to 2048 pixels per side and 1,048,576 total pixels. E6 and other non-standard screens remain unverified on hardware.
- **Chinese and English interfaces** on both the phone and PC.
- **RGB565 playback and import improvements:** up to 30 fps playback requests, including 360×640 at 20/30 fps. Importing reuses buffers, verifies data during copying and reduces progress-update overhead. Sustained frame rate and improved device import time have not been measured.
- **C/E/F wallpaper storage**, depending on the drives available on the device.
- **Renderer handshake checks:** detect an older injector still resident after an upgrade and prompt for a reboot.
- **Compressed MP4 wallpapers temporarily disabled:** import, playback and export are unavailable after E7 testing found that videos could open without producing a first frame, and cleanup could stop responding. MP4 source videos can still be converted to RGB565 SYWP on a PC. Existing MP4 library files remain available for deletion.

### Downloads and upgrading

| File | Purpose |
| --- | --- |
| `BelleWall-1.3.0.sisx` | Combined phone installer containing the injector, native process and app. This is the only installer most users need. |
| `BelleWall-1.3.0-injector.zip` | The same installer, a standalone injector and RGB565/web samples. Do not install the standalone injector again during normal setup. |
| `BelleWall-1.3.0-tools.zip` | Windows x64 Wallpaper Workshop with FFmpeg included. Node.js is required. Extract the full archive and run `Start-BelleWall.cmd` from its root folder. |

Before upgrading, stop playback and restore the desktop in the old version, wait for restoration to complete, then exit. After installation, **fully reboot the phone** before starting a wallpaper. Do not mix component versions.

The app installs on drive C; the wallpaper library can use E or F. Qt 4.7.4 or later, the required certificate trust and system permissions are needed.

### Verification scope

**2026-10-06: the user confirmed that release 1.3.0 works on Nokia E7.** Earlier device feedback also confirmed RGB565 playback, rotation and language switching. Version 1.3.0 includes MP4 disabling and the new icon. Sustained frame rate and battery life have not been measured.

Non-standard screen support has viewport detection and offline coverage, but this does not establish E6 hardware compatibility. Current 603 regression results are also pending. A 30 fps setting is a request limit, not a measured sustained frame rate; no new device import-time measurement is available.

Offline checks passed: all 66 tests, browser checks for the icon and bilingual conversion flow, SIS component identities, versions, dependencies, installation destinations, payloads and signatures, plus ZIP contents and file hashes.
