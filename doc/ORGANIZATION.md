# 目录整理

当前文档集中在 doc，根目录保留 README 和 LICENSE。doc/development 是研发记录，doc/history 是历史说明，doc/release 是此前审阅模板。prototype 下两个短跳转文件只为保留旧证据链接。

2026-09-27 将旧 build 日志、诊断解包、ROM 研究输出、旧审阅包和临时测试结果移入本地 archive/build-before-1.0-20260927，移动清单位于 archive/organization-20260927/moves.json。没有删除它们，历史 dist 检查点未改写。

保留当前使用的 ARM/native/helper 输出、FFmpeg、产品示例和签名目录；新发布构建位于 build/release-1.0.0。当前构建不依赖归档 ROM。需要重放历史实验时按清单恢复相应输入，不将其当作当前发布流程。

Git 不上传 build、dist、archive、research 原始证据／下载目录、系统解包、私人素材或签名私钥。发布源码、明确许可证的 vendor／baseline、doc、示例作者文件和工具。安装包与工具 ZIP 作为私有 Release 附件。

2026-09-27 视频候选整理：`dist` 保留 1.0.0 发布目录、已通过的 1.0.1 兼容性候选、当前 1.0.2 视频候选快照，以及可直接交付的 `BelleWall-1.0.2-injector-test.zip`。其余 63 个历史检查点目录和 47 个根部中间文件移至 `archive/dist-checkpoints-20260927`；`moves.json` 与 `root-file-moves.json` 记录原路径和新路径，没有删除原始文件。

随后按发布状态二次整理：`dist/release/<版本>/` 仅存正式安装包、PC 工具包及验证文件；`dist/test/<版本>/` 仅存测试候选；完整 1.0.1、1.0.2 构建快照移至 `build/checkpoints/`。`dist/release/1.0.0/` 原包未改动。两个通道可有相同版本号，须以通道、文件名和清单区分；GitHub Release 只使用 `release` 通道。
