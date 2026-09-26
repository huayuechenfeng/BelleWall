# BelleWall 1.0

Symbian Belle 动态壁纸：在原生桌面图标与小组件下面播放视频图片集或实时网页，支持管理、预览、暂停恢复和诊断。

## 下载与使用

在本仓库 Releases 下载 `BelleWall-1.0.0.sisx` 和 `BelleWall-1.0.0-tools-r3.zip`。仓库和 Release 保持私有，只有获授权账号可访问。

- [安装与卸载](doc/INSTALL.md)
- [新手：视频／预渲染 MPKG 制作壁纸](doc/BEGINNER.md)
- [进阶：自己编写网页壁纸](doc/WEB-AUTHORING.md)
- [SYWP 格式规范](doc/SYWP-FORMAT.md)
- [兼容性与已知限制](doc/COMPATIBILITY.md)
- [1.0 发布说明](doc/RELEASE-1.0.md)

制作工具本地运行，需要 Node.js；Windows x64 工具包已内置 FFmpeg，无需单独安装或设置路径。网页通过 bellewallStep(activeMs) 提供帧更新；暂停期间页面可以释放并重建。当前手机供帧约 10 fps，素材格式可表达更广的尺寸和帧率。

产品名称不限定机型，但底层仍有固件与权限检查，请先读兼容性说明。1.0 新封装完成离线校验，实机结果来自已验收运行基线；更长测试按用户要求取消。

## 开发

见 [构建说明](doc/BUILD.md) 和 [文档目录](doc/README.md)。prototype/src 是手机实现，prototype/tools 是制作和构建工具，prototype/tests 是离线测试，prototype/vendor 保留第三方来源，prototype/baseline 固定已验收渲染器。

原创部分采用 [MIT](LICENSE)，第三方保留原版权及许可，详见 [来源清单](doc/SOURCES-AND-LICENSES.md)。SDK、系统库、ROM、私钥、私人素材、历史 build 和设备原始日志不提交。
