# 代码来源、版权与许可证

BelleWall 原创源代码、制作工具、教程和原创示例采用 [MIT](../LICENSE)，版权署名为 2026 huayuechenfeng and BelleWall contributors。MIT 不覆盖第三方文件，也不改变私有 GitHub 仓库的访问控制。

| 部分 | 来源／固定版本 | 用途及许可证 |
|---|---|---|
| BelleWall src、tools、tests | 本项目实现 | MIT；生成固件指纹为识别常量，不包含 ROM 镜像 |
| h264bsd | [oneam/h264bsd](https://github.com/oneam/h264bsd)，42bcb5d753ad86d84903354bf3c68423c28adb7b | 历史视频兼容路径的解码器；AOSP 部分 Apache-2.0，附加代码 MIT；保留原文件头、PROVENANCE、LICENSE 和 Apache 全文 |
| Symbian homescreen 接口头 | [SymbianSource homescreen](https://github.com/SymbianSource/oss.FCL.sf.app.homescreen)，BRANCH_RCL_3，aa623cf29e16f66fa29b606da155c95a351b98f3 | 仅 hspswrapper/inc 与 idlehomescreen/inc 子集，未修改；Nokia 原版权及 EPL-1.0 保留，位于 prototype/vendor/symbian-homescreen |
| Qt / QtWebKit | 外部 Qt SDK／设备 Qt | 外部链接依赖；Qt 对应版本含 LGPL-2.1／GPL／商业许可选项及各第三方声明。本仓库不重新分发 Qt 二进制或宣称重新许可 Qt；保留 LGPL-2.1 文本供依赖说明 |
| Symbian SDK、GCCE | 用户本机工具链 | 外部构建依赖；SDK、系统库、ROM 和编译工具链不上传 |
| FFmpeg 9.0.1 | [官方源码](https://ffmpeg.org/releases/ffmpeg-9.0.1.tar.xz)，未修改源码的 Windows x64 精简构建 | tools-r3 内置独立 ffmpeg.exe，LGPL-2.1-or-later；完整对应源码、构建脚本、配置、哈希及许可证随包位于 ffmpeg/；未启用 GPL/nonfree 或外部编解码库 |
| MinGW-w64 / GCC runtime | MSYS2 UCRT64 构建工具链 | 内置转换器使用的编译器／运行库声明位于 ffmpeg/licenses；包含 GCC Runtime Library Exception 及 MinGW-w64 声明 |
| Node.js | 用户本机制作工具依赖 | 不捆绑二进制，依安装来源许可使用 |
| 色彩测试视频 | FFmpeg testsrc2 合成 | 无第三方壁纸作者作品；不是 Wallpaper Engine 官方素材 |
| clock.html、starter.html | 本项目 | MIT；可自行修改并打包 |

`LICENSES` 保存 MIT、Apache-2.0、EPL-1.0、LGPL-2.1 全文。第三方文件自身头部条款优先，不把混合来源目录整体重标为 MIT。h264bsd 来源说明位于其 vendor 目录；homescreen 头文件附固定提交和目录映射。

`prototype/baseline` 中四个 DLL／资源是本项目既有渲染器构建产物，不是手机 ROM 文件。保存哈希、源代码子集和来源检查点，1.0 继续复用已验收的二进制。重编译仍需本地 SDK，生成结果不自动继承实机验收。

历史研究曾参考 Qt Creator 2.4.1 的 CODA/TCF 协议实现和 Symbian 公开接口。当前 CODA PowerShell 工具为项目实现，不捆绑 Qt Creator 源代码。研究下载、解包 ROM、私人素材、签名私钥和原始设备日志均排除 Git 上传与 Release。

内置 FFmpeg 是独立命令行程序，通过文件和管道交换数据；其 LGPL 及运行库条款独立保留，不重标为 MIT。旧 tools / tools-r2 未捆绑 FFmpeg；tools-r3 使用项目精简构建，不是开发环境的 Gyan GPL essentials 构建。
