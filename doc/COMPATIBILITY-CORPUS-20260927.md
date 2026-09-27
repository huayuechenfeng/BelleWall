# BelleWall 桌面布局泛化：本地 ROM 样本检查

输入：`C:\linshi\rom\audit-output20260831-223653` 中的 8 套 CORE FPSX。只读提取版本资源和 `xn3layoutengine.dll`，按代码签名、Thumb 调用链、背景偏移、虚表与绘制函数检查。源 CORE 与提取 DLL 的 SHA-256 均已复核；完整路径和哈希见 [JSON 清单](compatibility-corpus-20260927.json)。

## Belle 目标固件

111/113 系列的五份镜像，其 `resource/versions/platform.txt` 均报告 `SymbianOSMajorVersion=101`。

| 机型 | 固件 | 选用路径 | 背景偏移 | DLL SHA-256 前 12 位 |
| --- | --- | --- | --- | --- |
| 603 | 111.020.0310 | generic fallback | 0x5c | `ac5c9952c293` |
| 700 | 111.020.0308 | generic fallback | 0x5c | `350c11e76968` |
| 701 | 111.030.0609 | exact E7 | 0x5c | `8750bd363a63` |
| 808 | 113.010.1508 | exact 603 FP2 | 0x5c | `dea5dd3d8687` |
| E7-00 | 111.040.1511 | exact E7 | 0x5c | `f2203cbd0e83` |

静态布局准入：5/5。其中 3 份命中既有精确布局，2 份使用通用回退。

## 非 Belle 负对照

以下三份 022/025 固件的 `resource/versions/platform.txt` 均报告 `SymbianOSMajorVersion=9`、`SymbianOSMinorVersion=5`。它们不计入 Belle 兼容率。

| 机型 | 固件 | 检查结果 | 背景偏移 | DLL SHA-256 前 12 位 |
| --- | --- | --- | --- | --- |
| C7-00 | 022.014 | 拒绝：View export changed | — | `d2854a2c124b` |
| N8-00 | 022.014 | 拒绝：View export changed | — | `d2854a2c124b` |
| X7-00 | 025.007 | 拒绝：View export changed | — | `3814fb0105a7` |

负对照拒绝：3/3。三份镜像的 `View()` 导出代码与 Belle 样本不同；C7 与 N8 的提取 DLL 哈希相同。这里的拒绝只针对所给旧固件镜像，不代表这些机型升级 Belle 后的结果。

这只证明静态布局检查的准入结果。真实设备上的载入重定位、屏幕与 Qt 条件、权限、播放和停止恢复仍需分别验证。
