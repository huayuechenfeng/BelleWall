# 构建

## 当前正式版 1.3.0

运行 `node prototype/tools/build-current-release.cjs`。Windows 上需配置 SDK／GCCE，并设置 `FFMPEG_BIN`、`BELLEWALL_TEST_FFMPEG` 和 `BELLEWALL_PORTABLE_FFMPEG`（路径同下方候选构建）。本入口统一版本 1.3.0，编译矢量图标 MIF、构建三个组件与正式组合安装包，运行离线测试及浏览器检查，封装 PC 与手机附件，校验发布清单。输出为 `dist/release/1.3.0/`，构建快照为 `build/checkpoints/BelleWall-1.3.0-release-v1/`；已有输出不会覆盖。发布仍是独立步骤，构建本身不会向 GitHub 上传或操作手机。

## 历史构建入口


从仓库根目录执行 `node prototype/tools/build-release.cjs`。这是历史 1.0.0 构建入口，默认输出 `build/release-checkpoints/BelleWall-1.0.0`；目录已存在时拒绝覆盖，传入另一个新路径可重建。正式交付目录是 `dist/release/<版本>/`，候选交付目录是 `dist/test/<版本>/`，完整构建快照放在 `build/checkpoints/`。

需要 Windows、Node.js、QtSDK SymbianSR1Qt474、GCCE 4、SDK makesis／signsis／dumpsis，以及 OpenSSL。手机目标运行环境的 Qt 为 4.8.1，编译 SDK 的 Qt 为 4.7.4。主机 C++ 测试需要 g++（默认 C:/msys64/usr/bin/g++.exe）。

1.0.1 兼容性候选使用 `node prototype/tools/build-compat-candidate.cjs <新的输出目录>`。该入口用 Nokia 603、E7 和 603 旧固件三份 ROM 验证桌面布局与通用特征的唯一命中，再编译两个渲染组件、原生程序和 Qt 程序，封装单一 SISX 并核对嵌套安装包。输出目录不能已存在；它不向手机安装或启动程序。候选包的 Qt 运行版本要求为编译版本 4.7.4；用户已反馈该候选包通过实机测试，通用回退路径仍待验收。`build-release.cjs` 是既有 1.0.0 发行流程，其说明与验收范围保持在本节其余内容中。

1.0.2 视频候选使用 `node prototype/tools/build-video-candidate.cjs <新的输出目录>`。它沿用布局验证与组合 SISX 核验，另生成 360×640 的 20 fps、30 fps 动态 SYWP 测试图，并记录包和样本哈希。当前离线构建在 `build/checkpoints/BelleWall-1.0.2-video-candidate-v2`；使用 `node prototype/tools/package-video-test.cjs <候选目录> <新的 ZIP 路径>` 生成手机测试 ZIP，交付位置为 `dist/test/1.0.2/`。用户反馈 E7 测试效果很好；603 本轮结果与持续帧率仍待确认。正式 1.0.2 使用 `node prototype/tools/promote-video-release.cjs` 从经核验的组件重新制作外层正式安装包，再封装当前 PC 工具。

1.0.3 双视频格式候选在 PowerShell 中设置 `BELLEWALL_CANDIDATE_VERSION=1.0.3`、`FFMPEG_BIN=<MP4V 便携版 ffmpeg/bin>` 和 `BELLEWALL_TEST_FFMPEG=<带 testsrc2 的开发机 ffmpeg.exe>`，再运行同一个 `build-video-candidate.cjs <新的输出目录>`。开发机 FFmpeg 只生成测试图；两种 SYWP 均由便携版转换。运行 `package-video-test.cjs <候选目录> <dist/test/1.0.3/…-injector-test.zip>` 可打包组合安装器、独立注入器和三种视频样本。设置 `BELLEWALL_PORTABLE_FFMPEG=<MP4V 便携版目录>` 后执行 `bundle-release-tools.cjs <候选目录/tools>`，再用 `package-release-tools.cjs <候选目录/tools> <dist/test/1.0.3/…-tools-test.zip>` 制作 PC 测试工具。1.0.3 是待 E7／603 验收的测试候选，不能放进 `dist/release/`。

环境变量 BELLE_SDK、BELLE_GCCE、OPENSSL、BELLEWALL_HOST_CXX 可覆盖工具路径。制作示例还需要 FFmpeg；FFMPEG_BIN 指向其 bin 目录。

构建执行主机测试、保留固定哈希的既有渲染器、构建 helper／native／Qt 应用、打包三个 1.0.0 组件，再合成 1.0.0 顶层 SISX。解包校验各载荷、UID、安装目标与版本依赖，保存签名检查、日志及哈希。三个组件保留原 UID 以支持升级。

`prototype/vendor` 保存明确许可的头文件和解码器源码；`prototype/baseline` 保存已验收渲染器。本入口不读取 research/sources、ROM 或旧 dist 检查点。签名证书／私钥由本地 ensure-signing 生成或复用在 build/signing，始终不提交。新证书不保证旧设备信任。

`build` 只保留当前构建输出、示例、外部工具和本地签名；归档索引见 [目录整理记录](ORGANIZATION.md)。旧研究命令在归档中追溯，不是当前构建入口。

## 内置 FFmpeg 的工具包

工具 ZIP 默认包含 Windows x64 本地转换器。先下载 `https://ffmpeg.org/releases/ffmpeg-9.0.1.tar.xz` 到 `build/ffmpeg-portable/`，SHA256 为 `cf38e0e28c7e5605942c4a77755349b0145804a397af37eb1fb4c77cb237f635`，然后解压到同目录。MSYS2 安装 `mingw-w64-ucrt-x86_64-gcc` 和 `make`、`diffutils`。在解压后的 FFmpeg 源码目录运行本仓库 `prototype/tools/build-portable-ffmpeg.sh`，完成后从仓库根目录运行：

```powershell
node prototype/tools/stage-portable-ffmpeg.cjs
node prototype/tools/bundle-release-tools.cjs build/release-1.0.0/tools-r3
```

暂存脚本默认读取 `C:/msys64` 的工具链版权文件，可用 `BELLEWALL_MSYS_ROOT` 覆盖；构建脚本在 MSYS2 UCRT64 中运行。工具包携带未修改的完整 FFmpeg 源码压缩包、同一构建脚本及配置；无网络协议、外部编解码库或 GPL/nonfree 功能。1.0.3 候选增加 FFmpeg 自带的 MPEG-4 Part 2 编码器与 MP4 muxer。不要把其他 FFmpeg 二进制直接替换进发行包却沿用该来源声明。

1.0.2 正式工具包使用 `node prototype/tools/bundle-release-tools.cjs build/release-1.0.2/tools`，再使用 `node prototype/tools/package-release-tools.cjs build/release-1.0.2/tools dist/release/1.0.2/BelleWall-1.0.2-tools.zip` 封装。ZIP 根目录直接包含 `Start-BelleWall.cmd`、README、`prototype/` 和 `ffmpeg/`，不增加外层目录。正式目录需保存 `CHANNEL.json`、`SHA256SUMS.txt`、验证清单与发布说明；运行 `node prototype/tools/verify-deliveries.cjs` 核对通道、文件名及哈希。GitHub Release 仅上传 `dist/release/<版本>/` 中的正式附件。


## 1.1.0 旋转与双语测试候选

在 PowerShell 设置 `BELLEWALL_CANDIDATE_VERSION=1.1.0`、`FFMPEG_BIN=<MP4V 便携版 bin>`、`BELLEWALL_TEST_FFMPEG=<开发机 FFmpeg>`，运行 `node prototype/tools/build-video-candidate.cjs build/checkpoints/BelleWall-1.1.0-display-language-candidate-v1`（重建时换一个未占用快照目录）。该脚本设置各组件相同版本，构建并验证组合安装包，生成九个视频／网页样本。不要使用未指定产品版本的底层历史构建命令来制作本轮包。

运行全量测试时设 `BELLEWALL_PRODUCT_VERSION=1.1.0`：`node --test prototype/tests/*.test.cjs`。`node prototype/tools/check-webui-browser.cjs` 检查双语切换、选项保留、两种视频导出、HTML 导出及窄屏布局。浏览器截图与结果位于 build/product-browser-qa。`node prototype/tools/check-docs.cjs` 校验文档链接及冻结基线。

沿用 package-video-test.cjs、bundle-release-tools.cjs 和 package-release-tools.cjs，输出到 `dist/test/1.1.0/`。交付目录只放两个测试 ZIP、README 和 CHANNEL.json，构建日志与独立组件保留在 build/checkpoints；生成 CHANNEL.json 后运行 verify-deliveries.cjs。ZIP 根目录直接放安装器或工具入口。正式 1.0.2 目录不变，本轮未发布正式 Release。

手机字符串源为 prototype/translations/phone-en.json，build.cjs 自动运行 generate-ui-strings.cjs 生成 C++ 表；translations.test.cjs 检查用户可见中文和占位符覆盖。PC 文案在 sywp-webui.html 的 zh/en 表中。

1.1.1 修复候选使用 `BELLEWALL_CANDIDATE_VERSION=1.1.1` 和快照 `build/checkpoints/BelleWall-1.1.1-redraw-handshake-v1`，同样运行 build-video-candidate.cjs。全量测试设置 `BELLEWALL_PRODUCT_VERSION=1.1.1`。本轮只交付新的 injector-test.zip，PC 制作工具继续使用 1.1.0 版。新握手头 renderreadiness.h 随 renderer 源码快照保存。安装后重启手机并核对 protocol=3.1 日志，参见 [修复说明](CANDIDATE-1.1.1.md)。

1.1.2 使用 `BELLEWALL_CANDIDATE_VERSION=1.1.2`，输出到 `build/checkpoints/BelleWall-1.1.2-rgb565-web-v1`。样本为七个 RGB565／网页包。测试与 PC 打包设置 `BELLEWALL_PRODUCT_VERSION=1.1.2`，使用现有 MP4V 便携 FFmpeg（输入解码继续保留），分别封装 injector-test.zip、tools-test.zip 到 `dist/test/1.1.2/`。公开转换入口拒绝 MP4 SYWP 输出，格式库保留历史 MP4 检查能力。
