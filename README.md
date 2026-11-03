# 图片工具箱 1.0.1

适用于 Windows 10 / 11 x64 的本地图片工具。双击 ImageToolkit.exe 即可使用，无需安装、无需编译。

## 功能

- 图片转 Base64：点击选图、拖入文件或粘贴图片；支持纯 Base64、Data URI、复制完整内容及保存 TXT。
- Base64 转图片：粘贴编码并预览、保存图片，可继续调整分辨率。
- 分辨率调整：自定义宽高、锁定比例、13 种预设、等比例补边或拉伸、背景色、透明背景及 JPG 质量设置。

三个功能通过左上角页签切换，采用浅色卡片界面。

## 使用

1. 图片转 Base64：点击左侧图片区选择文件，或拖入/粘贴图片，再点击“转换为 Base64”。文本区最多显示前 16000 个字符，复制和保存包含完整内容。
2. Base64 转图片：粘贴纯 Base64 或 Data URI，点击“解码并预览”，再保存。“继续调整分辨率”可将结果用于尺寸转换。
3. 分辨率调整：选择图片后设置尺寸、适配方式和格式，点击“转换并预览”，确认后另存为。

图片转 Base64 和分辨率页共用原图，重新选图或清空会清除相关旧结果。编辑 Base64 输入后需要重新解码。

## 格式与边界

- Base64 支持 PNG、JPG/JPEG、BMP、TIFF、GIF。粘贴的位图编码为 PNG。
- 分辨率输入额外支持 JFIF、ICO；输出支持 PNG、JPG、BMP、TIFF、GIF。
- GIF 动画、多页 TIFF 的预览及分辨率转换只处理第一帧。Base64 文件编码与解码保存保留原文件字节。
- 单个输入文件不超过 100 MB，总像素不超过 4000 万；输出宽高为 1–16384。
- 未包含 WebP、HEIC、AVIF 解码器。普通剪贴板位图可能不保留透明度。
- 图片全部在本机处理，不上传。

## 发布记录

1.0.1：采用标准 MinGW-w64 工具链构建，保留三个功能和美化后的界面。
本次正式发布的 EXE 与用户确认可用的 1.0.1 完全一致。

## 源码构建

使用 Linux x86-64、MinGW-w64 GCC 及 windres，执行 `bash build.sh`，输出为 `dist/ImageToolkit.exe`。使用软件无需构建。

可选验证：

```sh
python3 Native/test_base64.py
gcc -std=c11 -Wall -Wextra -Werror Native/test_geometry.c -o /tmp/image-toolkit-geometry
/tmp/image-toolkit-geometry
```
