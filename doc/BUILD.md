# 构建 1.0

从仓库根目录执行 `node prototype/tools/build-release.cjs`。默认输出 `dist/BelleWall-1.0.0`，目录已存在时拒绝覆盖；传入另一个新路径可重建。

需要 Windows、Node.js、QtSDK SymbianSR1Qt474、GCCE 4、SDK makesis／signsis／dumpsis，以及 OpenSSL。手机目标运行环境的 Qt 为 4.8.1，编译 SDK 的 Qt 为 4.7.4。主机 C++ 测试需要 g++（默认 C:/msys64/usr/bin/g++.exe）。

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

暂存脚本默认读取 `C:/msys64` 的工具链版权文件，可用 `BELLEWALL_MSYS_ROOT` 覆盖；构建脚本在 MSYS2 UCRT64 中运行。工具包携带未修改的完整 FFmpeg 源码压缩包、同一构建脚本及配置；无网络协议、外部编解码库或 GPL/nonfree 功能。不要把其他 FFmpeg 二进制直接替换进发行包却沿用该来源声明。
