# DLL Dependency Checker

一个基于Qt的Windows DLL依赖完备性检查工具。

## 功能特性

- ✅ 批量扫描文件夹中的所有DLL和EXE文件
- ✅ 单文件依赖关系分析
- ✅ 显示DLL的完整路径和版本信息
- ✅ 检测缺失的依赖项
- ✅ 架构兼容性检查（x86/x64）
- ✅ 循环依赖检测
- ✅ 开发机与目标机DLL清单对比
- ✅ 生成拷贝清单，简化部署流程
- ✅ 支持多种格式的报告导出（文本、HTML、CSV、JSON）
- ✅ 直观的树形界面展示依赖关系

## 系统要求

- Windows 7 / 10 / 11
- Qt 6.x 或更高版本（仅开发时需要）

## 快速开始

### 编译项目

1. 安装Qt开发环境
2. 打开Qt Creator
3. 打开 `DLLChecker.pro` 项目文件
4. 选择Release配置
5. 构建项目

### 部署

运行部署脚本自动收集依赖：
```batch
deploy.bat
```

详细的构建和部署说明请参考 [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md)

## 使用方法

### 扫描文件夹
1. 点击工具栏的"扫描文件夹"按钮
2. 选择要检查的目录
3. 查看依赖关系树和缺失的DLL

### 扫描单个文件
1. 点击工具栏的"扫描文件"按钮
2. 选择要检查的DLL或EXE文件
3. 查看完整的依赖树

### 开发机/目标机对比
1. **在开发机上：**
   - 扫描Release目录
   - 点击"导出报告"，选择JSON格式
   - 保存为清单文件（例如：dev_manifest.json）

2. **在目标机上：**
   - 扫描Release目录
   - 点击"加载清单"，选择开发机的清单文件
   - 查看缺失的DLL列表
   - 导出拷贝清单，了解需要从开发机拷贝哪些文件

### 导出报告
1. 扫描完成后，点击"导出报告"
2. 选择格式（文本、HTML、CSV）
3. 选择保存位置
4. 报告包含所有缺失的依赖和详细信息

## 界面说明

- **工具栏：** 提供扫描、加载清单、导出报告等功能
- **依赖树视图：** 显示文件的依赖关系，红色表示缺失，橙色表示架构不匹配
- **详细信息面板：** 显示选中DLL的详细信息（路径、版本、架构等）
- **状态栏：** 显示扫描进度和当前状态

## 技术特点

- 使用Windows API直接解析PE文件格式
- 不依赖第三方PE解析库
- 按照Windows DLL搜索顺序查找依赖
- 支持递归依赖扫描
- 自动检测循环依赖
- 智能缓存机制提升性能

## 项目结构

```
DLLChecker/
├── include/              # 头文件
│   ├── mainwindow.h
│   ├── peparser.h
│   ├── pathresolver.h
│   ├── dependencyscanner.h
│   ├── comparisonengine.h
│   └── reportgenerator.h
├── src/                  # 源文件
│   ├── main.cpp
│   ├── mainwindow.cpp
│   ├── peparser.cpp
│   ├── pathresolver.cpp
│   ├── dependencyscanner.cpp
│   ├── comparisonengine.cpp
│   └── reportgenerator.cpp
├── Release/              # 编译输出目录
├── DLLChecker.pro        # Qt项目文件
├── deploy.bat            # 部署脚本
├── BUILD_INSTRUCTIONS.md # 构建说明
└── README.md             # 本文件
```

## 核心模块

### PEParser
解析PE文件格式，提取依赖信息、架构信息和版本信息。

### PathResolver
按照Windows DLL搜索顺序查找DLL的实际位置。

### DependencyScanner
递归扫描文件的依赖关系，构建完整的依赖树。

### ComparisonEngine
对比开发机和目标机的DLL清单，生成拷贝列表。

### ReportGenerator
生成各种格式的检查报告。

### MainWindow
应用程序主界面，提供直观的用户交互。

## 常见使用场景

### 场景1：部署前检查
在将应用程序部署到目标机器前，使用本工具扫描Release目录，确保所有依赖都已包含。

### 场景2：解决缺失DLL问题
当应用程序在目标机器上报错缺少DLL时，使用本工具快速定位缺失的DLL及其来源。

### 场景3：架构兼容性检查
检查应用程序是否混用了x86和x64的DLL，避免加载失败。

### 场景4：依赖关系分析
了解应用程序的完整依赖关系，优化部署包大小。

## 开发状态

✅ 核心功能已完成并可用
- PE文件解析
- 依赖关系扫描
- 路径解析
- 清单对比
- 报告生成
- 图形界面

## 许可证

本项目仅供学习和个人使用。

## 贡献

欢迎提交问题和改进建议！

## 致谢

本项目使用以下技术：
- Qt Framework
- Windows PE Format
- Windows ImageHlp API
