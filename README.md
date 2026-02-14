# DLL Dependency Checker

一个基于Qt的Windows DLL依赖完备性检查工具，具有现代化的用户界面和强大的依赖分析功能。

## 功能特性

### 核心功能
- ✅ 批量扫描文件夹中的所有DLL和EXE文件
- ✅ 单文件依赖关系分析
- ✅ 显示DLL的完整路径和版本信息
- ✅ 检测缺失的依赖项
- ✅ 架构兼容性检查（x86/x64）
- ✅ 循环依赖检测
- ✅ 支持多种格式的报告导出（文本、HTML、CSV、JSON）

### 新工作流程
- ✅ **一键补齐**：自动收集并复制高亮的缺失DLL
- ✅ **差异报告**：开发机与目标机DLL清单对比
- ✅ **高亮显示**：自动高亮显示目标机缺失的DLL
- ✅ **冲突处理**：支持覆盖或跳过已存在文件的策略

### 界面优化
- ✅ **现代化设计**：采用现代化配色方案和圆角设计
- ✅ **图标支持**：工具栏按钮配备直观的图标
- ✅ **树形控件**：清晰的展开/折叠箭头图标
- ✅ **响应式布局**：优化的间距和边距
- ✅ **平滑动画**：展开/折叠时的流畅动画效果
- ✅ **悬停效果**：按钮和项目的悬停反馈

## 系统要求

- Windows 7 / 10 / 11
- Qt 5.x 或更高版本（开发时需要）
- CMake 3.16 或更高版本

## 快速开始

### 编译项目

#### 使用CMake
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

#### 使用Qt Creator
1. 打开Qt Creator
2. 打开 `CMakeLists.txt` 项目文件
3. 选择Release配置
4. 构建项目

详细的构建和部署说明请参考 [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md)

### 部署

运行部署脚本自动收集依赖：
```batch
deploy.bat
```

## 使用方法

### 新工作流程（推荐）

#### 场景：开发机 → 目标机部署

1. **在目标机上（缺少DLL的机器）：**
   - 运行DLL Checker
   - 点击"扫描文件夹"或"扫描文件"
   - 扫描后点击"导出缺失报告"
   - 保存为 `missing_report.json`
   - 将此文件复制到开发机

2. **在开发机上（有完整DLL的机器）：**
   - 运行DLL Checker
   - 扫描相同的文件夹/文件
   - 点击"导入差异报告"，选择 `missing_report.json`
   - 系统会自动高亮显示缺失的DLL（黄色背景）
   - 点击"一键补齐"，选择目标文件夹
   - 选择冲突处理策略（全部覆盖/全部跳过）
   - 自动收集并复制所有缺失的DLL

3. **部署：**
   - 将收集的DLL文件夹复制到目标机
   - 完成部署！

### 传统扫描功能

#### 扫描文件夹
1. 点击工具栏的"扫描文件夹"按钮 📁
2. 选择要检查的目录
3. 查看依赖关系树和缺失的DLL

#### 扫描单个文件
1. 点击工具栏的"扫描文件"按钮 📄
2. 选择要检查的DLL或EXE文件
3. 查看完整的依赖树

#### 界面操作
- **展开/折叠**：点击箭头图标或项目名称
- **查看详情**：点击任意项目查看详细信息
- **筛选系统DLL**：勾选"显示系统DLL"复选框
- **递归扫描**：勾选"递归扫描"复选框
- **取消扫描**：点击"取消扫描"按钮中断当前操作

### 导出报告

1. 扫描完成后，点击"导出缺失报告"按钮 📤
2. 选择保存位置（JSON格式）
3. 报告包含所有缺失的依赖和详细信息

## 界面说明

- **工具栏**：提供扫描、导入/导出、一键补齐等功能，每个按钮都有图标和工具提示
- **依赖树视图**：
  - 显示文件的依赖关系树
  - 🔴 红色：缺失的DLL
  - 🟠 橙色：架构不匹配
  - 🟡 黄色背景：高亮的缺失DLL（导入差异报告后）
  - ▶/▼ 箭头：展开/折叠子项目
- **详细信息面板**：显示选中DLL的详细信息（路径、版本、架构、大小等）
- **状态栏**：显示扫描进度、剩余时间和当前状态

## 技术特点

- 使用Windows API直接解析PE文件格式
- 不依赖第三方PE解析库
- 按照Windows DLL搜索顺序查找依赖
- 支持递归依赖扫描
- 自动检测循环依赖
- 智能缓存机制提升性能
- 多线程扫描，避免界面卡顿
- 完整的输入验证和错误处理
- 现代化的QSS样式表美化

## 项目结构

```
DLLChecker/
├── include/              # 头文件
│   ├── mainwindow.h
│   ├── peparser.h
│   ├── pathresolver.h
│   ├── dependencyscanner.h
│   ├── comparisonengine.h
│   ├── reportgenerator.h
│   ├── dllcollector.h
│   ├── scanworker.h
│   ├── inputvalidator.h
│   └── logger.h
├── src/                  # 源文件
│   ├── main.cpp
│   ├── mainwindow.cpp
│   ├── peparser.cpp
│   ├── pathresolver.cpp
│   ├── dependencyscanner.cpp
│   ├── comparisonengine.cpp
│   ├── reportgenerator.cpp
│   ├── dllcollector.cpp
│   ├── scanworker.cpp
│   ├── inputvalidator.cpp
│   └── logger.cpp
├── resources/            # 资源文件
│   ├── style.qss        # 样式表
│   ├── resources.qrc    # Qt资源文件
│   └── icons/          # SVG图标
├── tests/               # 测试文件
├── CMakeLists.txt       # CMake构建文件
├── deploy.bat          # 部署脚本
├── BUILD_INSTRUCTIONS.md # 构建说明
├── NEW_WORKFLOW_GUIDE.md # 新工作流程指南
└── README.md           # 本文件
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

### DLLCollector
自动收集并复制高亮的缺失DLL。

### ScanWorker
多线程工作线程，执行扫描任务避免界面卡顿。

### InputValidator
输入验证，提供友好的错误提示和建议。

### Logger
日志记录系统，便于调试和问题排查。

### MainWindow
应用程序主界面，提供直观的用户交互。

## 常见使用场景

### 场景1：开发机到目标机部署（新工作流程）
这是推荐的使用方式，可以快速补齐目标机缺失的DLL：
1. 目标机扫描并导出缺失报告
2. 开发机导入报告并一键补齐
3. 部署收集的DLL

### 场景2：部署前检查
在将应用程序部署到目标机器前，使用本工具扫描Release目录，确保所有依赖都已包含。

### 场景3：解决缺失DLL问题
当应用程序在目标机器上报错缺少DLL时，使用本工具快速定位缺失的DLL及其来源。

### 场景4：架构兼容性检查
检查应用程序是否混用了x86和x64的DLL，避免加载失败。

### 场景5：依赖关系分析
了解应用程序的完整依赖关系，优化部署包大小。

## 开发状态

✅ 核心功能已完成并可用
- PE文件解析
- 依赖关系扫描
- 路径解析
- 清单对比
- 报告生成
- 图形界面
- 多线程扫描
- 现代化UI美化
- 新工作流程

## 许可证

本项目仅供学习和个人使用。

## 贡献

欢迎提交问题和改进建议！

## 致谢

本项目使用以下技术：
- Qt Framework
- Windows PE Format
- Windows ImageHlp API
- CMake Build System

## 相关文档

- [构建说明](BUILD_INSTRUCTIONS.md)
- [新工作流程指南](NEW_WORKFLOW_GUIDE.md)
- [使用示例](USAGE_EXAMPLES.md)
- [项目总结](PROJECT_SUMMARY.md)
