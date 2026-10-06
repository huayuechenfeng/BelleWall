# BelleWall 1.1.1 刷新通道修复候选

## 问题与证据

E7 用户反馈升级 1.1.0 后，包括原本可播放的 RGB565 壁纸在内，桌面不动且无报错。2026-10-06 通过 CODA 读取的日志显示：一次 60 秒会话发布 1199 帧，注入器报告 1080 次检查，但 `DIRTY requests=0`；停止后恢复成功。安装记录中的程序、原生播放器和注入器均为 1.1.0，用户确认升级后没有重启。

1.1.0 将刷新通道从 V2 改为 V3，却仍沿用原桌面会话版本和注入器标识。旧 DLL 若驻留在桌面进程中，可以继续提供旧会话心跳，但无法接收 V3 帧。原启动判断仅检查心跳，因而不能发现这种不配套状态。驻留旧 DLL 是当前最符合日志的解释；重启后的播放结果仍须实机确认，安装版本号本身不能证明进程已加载新版 DLL。

同一份日志还出现一次 `E32USER-CBase / 46`。SDK 将其定义为 active scheduler 的 stray signal；播放器的 helper 等待使用裸 Logon 加轮询，正常完成路径没有消费完成通知，在进入 MP4 异步调度器时存在此风险。

## 本轮修改

- 播放前要求注入器通过当前刷新通道明确确认协议 3.1，不能再仅凭会话心跳进入播放。
- 桌面前台连续等待约 5 秒仍无确认，则停止并走原恢复事务。应用显示错误码 -7110，提示重启手机或重新安装配套组件；锁屏和离开桌面的时间不计入这 5 秒。
- 注入器构造日志包含 `redraw=3.1`，播放器记录握手成功或失败，便于确认实际运行组件。
- helper 等待改为带超时的 WaitForRequest，正常路径消费完成通知，避免遗留裸 Logon 信号进入 MP4 调度器。
- 保留 1.1.0 的旋转、通用尺寸、双语和视频功能，本轮没有改变固件偏移。

## 安装与验收

先停止并恢复桌面，再安装 ZIP 根目录的 `BelleWall-1.1.1-video-candidate.sisx`。组合包内含三个同版本组件，独立 `injector/bellerender-selfsigned.sisx` 仅供调试。安装后完整重启手机，让桌面加载新注入器。壁纸库继续保留。

制作工具继续使用 1.1.0 的 tools-test.zip，SYWP 格式没有变化。先用原来正常的 RGB565 壁纸做 60 秒检查，确认能实际刷新且停止后恢复；再测试 MP4 和横竖切换。导出的日志应出现 `REDRAW current transport handshake accepted protocol=3.1`，注入器结束时 `DIRTY requests` 应大于 0。计数代表已发起桌面绘制，不等于 LCD 实测帧率。

本包为修复候选，不是正式 Release。仅编译和离线测试不能替代上述实机验证。

## English

Stop playback and restore the desktop, install the combined root SISX, then fully restart the phone to load the new desktop plugin. Use the existing 1.1.0 workshop. This fix requires a redraw-transport handshake before playback; a legacy resident plugin can no longer pass readiness using only its session heartbeat. Error -7110 requests a restart or matching component reinstall. Helper process completion is also consumed before the MP4 active scheduler starts. Verify an existing RGB565 wallpaper first, then MP4 and rotation. Device acceptance remains pending.
