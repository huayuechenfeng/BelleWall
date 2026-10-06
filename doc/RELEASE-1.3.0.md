# BelleWall 1.3 正式版 / Release 1.3

安装包与三个组件统一版本为 **1.3.0**，保持原 UID，可从旧版升级。

## 本次更新

- 新增 Belle 风格图标：薄荷绿与深蓝绿的双层圆角方块，手机菜单与 PC 制作工具统一使用。
- 支持横竖屏切换时暂停、重建画面并恢复播放，以及 cover／contain／stretch 和方向限制。
- 支持非标准画布与响应式网页；单边最多 2048、总像素最多 1,048,576。E6 等非标准屏尚未实机验证。
- 手机与制作工具支持中文／English。
- RGB565 视频最高请求 30 fps，支持 360×640、20／30 fps；导入使用复用缓冲区、复制时校验与节流进度更新。30 fps 是请求上限，不是实测持续帧率；大文件导入优化尚无新的实机耗时记录。
- 壁纸库可选择 C／E／F 盘（以设备实际可用盘符为准）。
- 增加刷新通道握手，检测升级后残留的旧注入器并提示重启。
- 暂停 MP4 压缩壁纸的导入、播放与导出：E7 上可打开视频但无法输出首帧，退出流程也可能失去响应。MP4 等源视频仍可在 PC 转换为 RGB565 SYWP。已有 MP4 壁纸保留供删除。

## 下载与升级

- `BelleWall-1.3.0.sisx`：手机组合安装包，已包含注入器、原生进程与主程序，正常使用只需安装这一个文件。
- `BelleWall-1.3.0-injector.zip`：同一安装包、独立注入器与 RGB565／网页样例，供需要完整手机附件的用户使用；不要重复安装独立注入器。
- `BelleWall-1.3.0-tools.zip`：Windows x64 制作工具，内置 FFmpeg，需自行安装 Node.js。解压后根目录直接运行 `Start-BelleWall.cmd`。

升级前先在旧版“停止并恢复桌面”，确认恢复并退出。安装后**完整重启手机**，再启动壁纸。不要混用不同版本组件。程序安装于 C 盘；壁纸库可放 E／F 盘。Qt 最低 4.7.4，设备须具备相应证书信任与系统权限。

## 验证范围

用户已在 E7 的 1.1 系列实现上确认 RGB565 播放、横屏及双语正常；1.3.0 沿用该实现并整合 MP4 禁用与新图标。新 1.3.0 安装包尚未单独在真机安装验收。非标准屏仅有尺寸识别／离线覆盖，不能据此认定 E6 已兼容；603 本轮结果待反馈。构建时核验三个组件的 UID、版本、依赖、安装位置、载荷、签名及 ZIP 内容。

## English

BelleWall 1.3.0 adds the new stacked-square icon, rotation rebuilding, screen fitting, Chinese/English UI, faster RGB565 import processing, C/E/F wallpaper storage, and renderer handshake checks. Video playback requests up to 30 fps; sustained device frame rate and improved import time are not measured.

Compressed MP4 wallpapers are temporarily disabled after an E7 frame extraction/cleanup failure. MP4 input videos can still be converted to RGB565 SYWP in the PC Workshop. Existing MP4 library files are not automatically deleted.

Stop playback and restore the desktop before upgrading, install the combined SISX, then fully reboot the phone. All three components are version 1.3.0. Qt 4.7.4 or later and the required device permissions are needed. E7 playback, rotation and language switching were user-tested on the preceding 1.1 implementation; the new 1.3.0 installer itself, 603 regression tests and E6 hardware behavior remain unverified.
