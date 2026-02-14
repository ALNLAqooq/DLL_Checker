# DLL Dependency Checker - 构建和部署说明

## 系统要求

### 开发环境
- Windows 7 / 10 / 11
- Qt 6.x 或更高版本（推荐 Qt 6.5+）
- Visual Studio 2019 或更高版本（或 MinGW）
- CMake 3.16+ （可选）

### 目标环境
- Windows 7 / 10 / 11
- 无需安装Qt运行时（所有依赖都会打包）

## 构建步骤

### 方法1：使用Qt Creator（推荐）

1. 打开Qt Creator
2. 打开项目文件 `DLLChecker.pro`
3. 选择合适的Kit（例如：Desktop Qt 6.5.0 MSVC2019 64bit）
4. 在左侧选择 "Release" 构建配置
5. 点击 "构建" 按钮（或按 Ctrl+B）
6. 构建完成后，可执行文件位于 `Release/DLLChecker.exe`

### 方法2：使用命令行

```batch
REM 设置Qt环境变量
set PATH=C:\Qt\6.5.0\msvc2019_64\bin;%PATH%

REM 生成Makefile
qmake DLLChecker.pro -spec win32-msvc "CONFIG+=release"

REM 编译
nmake release

REM 或者使用MinGW
REM qmake DLLChecker.pro -spec win32-g++ "CONFIG+=release"
REM mingw32-make release
```

## 部署步骤

### 自动部署（推荐）

1. 确保已经在Release模式下构建了项目
2. 确保Qt的bin目录在PATH环境变量中
3. 运行部署脚本：
   ```batch
   deploy.bat
   ```
4. 脚本会自动收集所有Qt依赖到Release目录

### 手动部署

1. 打开命令提示符
2. 切换到项目目录
3. 运行windeployqt：
   ```batch
   cd Release
   windeployqt DLLChecker.exe --release --no-translations
   ```

### 部署后的目录结构

```
Release/
├── DLLChecker.exe          # 主程序
├── Qt6Core.dll             # Qt核心库
├── Qt6Widgets.dll          # Qt界面库
├── Qt6Gui.dll              # Qt图形库
└── platforms/              # Qt平台插件
    └── qwindows.dll        # Windows平台插件
```

## 在目标机器上运行

1. 将整个 `Release` 文件夹复制到目标机器
2. 双击 `DLLChecker.exe` 运行
3. 无需安装任何额外的运行时

## 常见问题

### Q: 程序无法启动，提示缺少DLL
A: 确保运行了 `windeployqt` 或 `deploy.bat` 脚本

### Q: 在Windows 7上无法运行
A: 确保使用的Qt版本支持Windows 7（Qt 6.5之前的版本）

### Q: 编译时出现链接错误
A: 检查是否正确链接了 imagehlp.lib 和 version.lib

### Q: 如何减小程序体积
A: 可以使用UPX压缩可执行文件，或者静态链接Qt库

## 测试建议

在部署前，建议在以下环境中测试：

1. 干净的Windows 7虚拟机（无Qt运行时）
2. 干净的Windows 10虚拟机
3. Windows 11系统

确保程序能够正常启动和运行所有功能。

## 技术支持

如有问题，请检查：
1. Qt版本是否正确
2. 编译器版本是否匹配
3. 是否正确部署了所有依赖
