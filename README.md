# BelleWall 1.3

Symbian Belle 动态壁纸：在原生桌面图标与小组件下面播放视频图片集或实时网页，支持管理、预览、暂停恢复和诊断。

![BelleWall](assets/branding/bellewall-icon.svg)

## 下载与使用

在本仓库 Releases 下载 `BelleWall-1.3.0.sisx` 和 `BelleWall-1.3.0-tools.zip`。仓库和 Release 保持私有，只有获授权账号可访问。测试候选单独位于本地 `dist/test/`，不作为正式 Release 附件。

- [安装与卸载](doc/INSTALL.md)
- [新手：视频／预渲染 MPKG 制作壁纸](doc/BEGINNER.md)
- [进阶：自己编写网页壁纸](doc/WEB-AUTHORING.md)
- [SYWP 格式规范](doc/SYWP-FORMAT.md)
- [兼容性与已知限制](doc/COMPATIBILITY.md)
- [1.3 正式版发布说明](doc/RELEASE-1.3.0.md)
- [1.0 发布说明](doc/RELEASE-1.0.md)
- [1.0.2 发布说明](doc/RELEASE-1.0.2.md)
- [1.1.2 测试候选：暂时关闭 MP4 壁纸](doc/CANDIDATE-1.1.2.md)
- [1.1.1 刷新通道修复候选](doc/CANDIDATE-1.1.1.md)
- [1.1.0 测试候选与验收清单](doc/CANDIDATE-1.1.0.md)
- [下一阶段计划](doc/NEXT-PHASE-PLAN.md)

制作工具本地运行，需要 Node.js；Windows x64 工具包已内置 FFmpeg，无需单独安装或设置路径。网页通过 bellewallStep(activeMs) 提供帧更新；暂停期间页面可以释放并重建。当前视频请求帧率最高 30 fps，网页约 10 fps；实际持续帧率依设备而定。

产品名称不限定机型，但底层仍有固件与权限检查，请先读兼容性说明。1.3 整合横屏重建、非标准画布和中英界面。用户已在 E7 的 1.1 系列实现上确认播放、横屏与双语正常；1.3.0 新包、603 本轮回归及 E6 真机效果仍待验证。

## 开发

见 [构建说明](doc/BUILD.md) 和 [文档目录](doc/README.md)。prototype/src 是手机实现，prototype/tools 是制作和构建工具，prototype/tests 是离线测试，prototype/vendor 保留第三方来源，prototype/baseline 固定已验收渲染器。

原创部分采用 [MIT](LICENSE)，第三方保留原版权及许可，详见 [来源清单](doc/SOURCES-AND-LICENSES.md)。SDK、系统库、ROM、私钥、私人素材、历史 build 和设备原始日志不提交。

当前 1.3 正式版支持 RGB565／网页壁纸，暂时关闭手机 MP4 壁纸及对应导出。MP4 视频素材仍可通过 PC 工具转换为 RGB565。E7 用户已确认横屏和双语正常，非标准屏尚无实机验证。
