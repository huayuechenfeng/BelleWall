<p align="center">
  <img src="assets/branding/bellewall-icon.svg" width="104" height="104" alt="BelleWall 图标">
</p>

<h1 align="center">BelleWall</h1>

<p align="center">让你的 Symbian Belle 桌面动起来。</p>

<p align="center">简体中文 · <a href="README.en.md">English</a></p>

<p align="center">
  <a href="https://github.com/huayuechenfeng/BelleWall/releases/latest">下载正式版</a> ·
  <a href="doc/BEGINNER.md">制作第一张壁纸</a> ·
  <a href="doc/INSTALL.md">安装与升级</a>
</p>

BelleWall 可以在手机原生桌面的图标和小组件下播放动态壁纸。把喜欢的视频转换为壁纸包，或使用实时网页壁纸，在保留原有桌面操作方式的同时，让背景动起来。

## 可以做什么

- **视频动态壁纸**：将 MP4 等视频素材转换为 `.sywp`，支持 20／30 fps 设置和 360×640 画布。
- **转换 Wallpaper Engine 壁纸**：使用制作工具，将 Wallpaper Engine 导出的预渲染视频壁纸（视频型 MPKG）转换为 BelleWall 可用的 `.sywp`。
- **网页动态壁纸**：使用时钟等实时网页内容，也可以自己编写壁纸。
- **横竖屏切换**：随屏幕方向调整画面，可选择铺满裁剪、完整显示或拉伸。
- **暂停与恢复**：锁屏或离开桌面时暂停，返回后恢复；停止播放后恢复原生背景。
- **选择存储盘**：导入壁纸时选择 C、E 或 F 盘，方便把较大的文件放在有空间的盘上。
- **中文与 English**：手机程序和电脑制作工具均支持双语。

视频当前使用 RGB565 图片帧播放，文件会比原视频大。30 fps 是播放请求上限，实际流畅度取决于手机与壁纸内容。

## 壁纸演示

三款免费像素动画壁纸，可直接导入 BelleWall 1.3：

| 霓虹城市 | 月夜群山 | 像素星球 |
| --- | --- | --- |
| ![霓虹城市动画](assets/wallpapers/previews/neon-city.gif) | ![月夜群山动画](assets/wallpapers/previews/moonlit-mountains.gif) | ![像素星球旋转动画](assets/wallpapers/previews/pixel-planet.gif) |
| [下载 SYWP](https://github.com/huayuechenfeng/BelleWall/releases/download/v1.3.0/BelleWall-Neon-City.sywp) | [下载 SYWP](https://github.com/huayuechenfeng/BelleWall/releases/download/v1.3.0/BelleWall-Moonlit-Mountains.sywp) | [下载 SYWP](https://github.com/huayuechenfeng/BelleWall/releases/download/v1.3.0/BelleWall-Pixel-Planet.sywp) |
| 8 秒 · 70.3 MiB | 8 秒 · 70.3 MiB | 7.7 秒 · 67.7 MiB |

均为 **360×640、20 fps 的 RGB565 帧壁纸**。复制到手机后，在**壁纸库**中导入并启动。建议选择 E/F 盘，并为导入后的副本预留空间。横屏保持完整画面，空余区域留黑边。

上图是实际转换帧缩小到 180×320、10 fps 的 GIF 预览，并非手机实拍。文件格式与循环检查已通过，这三款壁纸尚待手机实测。原始美术均为 **CC0**：[素材来源、署名及重新生成方法](assets/wallpapers/CREDITS.txt)。

## 下载

当前正式版：**1.3.0** · [本版更新说明](doc/RELEASE-1.3.0.md)

打开 [GitHub Release 下载页](https://github.com/huayuechenfeng/BelleWall/releases/latest)，在 **Assets** 中选择：

| 你需要做什么 | 下载哪个文件 |
| --- | --- |
| 在手机上安装或升级 BelleWall | **`BelleWall-1.3.0.sisx`** |
| 在 Windows 电脑上制作壁纸 | **`BelleWall-1.3.0-tools.zip`** |
| 同时获取手机安装包、壁纸样例和独立注入器 | `BelleWall-1.3.0-injector.zip`（可选） |

**通常下载 SISX 和 tools ZIP 即可。** SISX 已包含全部手机组件，无需再单独安装注入器。GitHub 自动提供的 `Source code` 是程序源码，安装使用请选择上表文件。

## 开始使用

### 1. 安装到手机

需要运行 **Symbian Belle** 的手机、**Qt 4.7.4 或更高版本**，以及程序运行所需的系统权限。安装包不包含 Qt；不同固件的可用情况见下方“机型兼容性”。

1. 如果已安装旧版，先在 BelleWall 中选择“停止并恢复桌面”，完成后退出。
2. 将 `BelleWall-1.3.0.sisx` 复制到手机，选择 C 盘安装。
3. **完整重启手机**，再打开 BelleWall。
4. 初次使用前，将桌面各页背景设为原生默认黑色背景。

程序安装在 C 盘，导入的壁纸可以另外存放到 E／F 盘。详细操作见 [安装、升级与卸载](doc/INSTALL.md)。

### 2. 制作一张壁纸

电脑制作工具适用于 **Windows 10／11 64 位**，需要先安装 Node.js，FFmpeg 已内置。

1. 完整解压 `BelleWall-1.3.0-tools.zip`，双击根目录的 `Start-BelleWall.cmd`。
2. 在浏览器打开启动窗口显示的网址，制作期间保持该窗口开启。
3. 选择视频素材，或 Wallpaper Engine 导出的预渲染视频壁纸 MPKG，调整画布、裁剪和帧率。第一次可以使用工具包里的 `examples/demo.mp4`。
4. 点击“生成壁纸包”，下载得到的 `.sywp` 文件。

首次尝试可用 **180×320、10 fps、100 张**，制作一段约 10 秒的轻量循环。确认播放正常后，再增加尺寸或帧率。素材在电脑本地处理，无需上传。

已有 `.sywp` 壁纸的用户可以直接进行下一步。完整教程见 [不写代码制作动态壁纸](doc/BEGINNER.md)。

### 3. 导入并播放

1. 将 `.sywp` 复制到手机。
2. 打开 BelleWall → **壁纸管理 → 导入壁纸包**，选择文件和存储盘。
3. 预览壁纸，点击 **启动动态壁纸**，第一次先选择 **60 秒检查**。
4. 准备期间保持手机解锁，等待自动切页完成；确认正常后即可使用持续播放。

导入只会添加壁纸，**还需要点击启动**才会改变桌面。要更换壁纸或结束播放，请先选择“停止并恢复桌面”。

## 机型兼容性

所有机型使用同一个安装包。BelleWall 面向 Symbian Belle，但实际可用性仍取决于固件、权限和桌面环境。

- **Nokia E7**：已有播放、横屏切换及双语界面正常的用户反馈。
- **Nokia 603**：有早期版本实机验证，当前版本的回归测试仍待完成。
- **Nokia E6 及其他机型**：已加入非标准画布适配，但不能据此保证每台设备都可用；E6 尚无实机验证。

E7 的上述反馈来自前序 1.1 系列；新 1.3.0 安装包尚未单独完成实机验收。更多信息见 [兼容性与验证范围](doc/COMPATIBILITY.md)。

## 常见问题

### 可以直接用 MP4 当壁纸吗？

**当前不能直接播放 MP4 压缩壁纸。** 你仍可以在电脑制作工具中选择 MP4 素材，转换为 RGB565 格式的 `.sywp` 后导入手机。网页壁纸也继续支持。

### 为什么壁纸文件比原视频大，导入也需要时间？

RGB565 壁纸保存的是未压缩图片帧，以减少手机播放时的视频解码开销。360×640、30 fps、10 秒的图片数据约为 **132 MiB**。可以缩短循环、降低分辨率或帧率，并在导入时选择空间充足的 E／F 盘。

### 安装或导入后，桌面没有动怎么办？

先确认已经点击“启动动态壁纸”。如果刚升级过程序，请完整重启手机，再运行“60 秒检查”。仍有问题时，在“运行诊断”中导出日志，反馈手机型号、固件版本、壁纸类型和错误提示。

### 能直接转换 Wallpaper Engine 壁纸吗？

可以。**制作工具支持将 Wallpaper Engine 导出的预渲染视频壁纸（含视频入口的 MPKG）转换为 BelleWall 的 `.sywp` 壁纸**，转换后复制到手机并导入即可。实时场景、粒子、模型或 SWF 不能直接转换，请先在原工具中录制或预渲染为视频。详见 [素材选择说明](doc/BEGINNER.md)。

### 如何停止或卸载？

停止播放时选择“停止并恢复桌面”。卸载前还需执行“卸载准备”，确认成功后再到系统应用管理移除 BelleWall。具体步骤见 [卸载说明](doc/INSTALL.md)。

## 更多资料

- [编写自己的网页壁纸](doc/WEB-AUTHORING.md)
- [1.3 更新说明](doc/RELEASE-1.3.0.md)
- [完整文档目录](doc/README.md)

开发与贡献：[构建说明](doc/BUILD.md) · [SYWP 格式规范](doc/SYWP-FORMAT.md)

原创代码采用 [MIT 许可证](LICENSE)，第三方组件保留各自许可，见 [来源与许可证](doc/SOURCES-AND-LICENSES.md)。
