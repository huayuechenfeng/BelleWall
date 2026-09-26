# 构建 1.0

从仓库根目录执行 `node prototype/tools/build-release.cjs`。默认输出 `dist/BelleWall-1.0.0`，目录已存在时拒绝覆盖；传入另一个新路径可重建。

需要 Windows、Node.js、QtSDK SymbianSR1Qt474、GCCE 4、SDK makesis／signsis／dumpsis，以及 OpenSSL。手机目标运行环境的 Qt 为 4.8.1，编译 SDK 的 Qt 为 4.7.4。主机 C++ 测试需要 g++（默认 C:/msys64/usr/bin/g++.exe）。

环境变量 BELLE_SDK、BELLE_GCCE、OPENSSL、BELLEWALL_HOST_CXX 可覆盖工具路径。制作示例还需要 FFmpeg；FFMPEG_BIN 指向其 bin 目录。

构建执行主机测试、保留固定哈希的既有渲染器、构建 helper／native／Qt 应用、打包三个 1.0.0 组件，再合成 1.0.0 顶层 SISX。解包校验各载荷、UID、安装目标与版本依赖，保存签名检查、日志及哈希。三个组件保留原 UID 以支持升级。

`prototype/vendor` 保存明确许可的头文件和解码器源码；`prototype/baseline` 保存已验收渲染器。本入口不读取 research/sources、ROM 或旧 dist 检查点。签名证书／私钥由本地 ensure-signing 生成或复用在 build/signing，始终不提交。新证书不保证旧设备信任。

`build` 只保留当前构建输出、示例、外部工具和本地签名；归档索引见 [目录整理记录](ORGANIZATION.md)。旧研究命令在归档中追溯，不是当前构建入口。
