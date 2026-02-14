# DLL Dependency Checker - 使用示例

本文档提供详细的使用示例，帮助你快速上手DLL依赖检查工具。

## 示例1：检查Qt应用程序的依赖

### 场景
你开发了一个Qt应用程序，想要检查Release目录中是否包含所有必要的DLL。

### 步骤
1. 启动DLL Dependency Checker
2. 点击"扫描文件夹"按钮
3. 选择你的应用程序的Release目录（例如：`C:\MyApp\Release`）
4. 等待扫描完成

### 结果解读
- **绿色/正常显示**：DLL存在且架构匹配
- **红色 [MISSING]**：DLL缺失，需要添加
- **橙色 [ARCH MISMATCH]**：架构不匹配（例如x64程序依赖x86 DLL）

### 示例输出
```
MyApp.exe [x64] v1.0.0.0
  └─ Qt6Core.dll [x64] v6.5.0
  └─ Qt6Widgets.dll [x64] v6.5.0
  └─ Qt6Gui.dll [x64] v6.5.0
  └─ MyCustomLib.dll [MISSING]  ← 需要添加这个DLL
```

## 示例2：单个DLL的依赖分析

### 场景
你有一个第三方DLL，想要了解它依赖哪些其他DLL。

### 步骤
1. 点击"扫描文件"按钮
2. 选择要分析的DLL（例如：`ThirdParty.dll`）
3. 查看完整的依赖树

### 示例输出
```
ThirdParty.dll [x86] v2.1.0
  └─ msvcr120.dll [x86] v12.0.0
  └─ msvcp120.dll [x86] v12.0.0
  └─ CustomHelper.dll [MISSING]
      └─ (无法继续扫描，因为文件缺失)
```

## 示例3：开发机到目标机的部署

### 场景
你在开发机上完成了应用程序开发，现在需要部署到一台干净的Windows 7目标机器上。

### 在开发机上的操作

1. **扫描并生成清单**
   - 启动DLL Dependency Checker
   - 扫描你的Release目录
   - 点击"导出报告"
   - 选择JSON格式
   - 保存为 `dev_manifest.json`

2. **清单文件示例**
```json
{
  "generated_time": "2026-01-14T10:30:00",
  "scan_root": "C:/MyApp/Release",
  "dlls": [
    {
      "name": "MyApp.exe",
      "path": "C:/MyApp/Release/MyApp.exe",
      "version": "1.0.0.0",
      "arch": "x64"
    },
    {
      "name": "Qt6Core.dll",
      "path": "C:/Qt/6.5.0/msvc2019_64/bin/Qt6Core.dll",
      "version": "6.5.0.0",
      "arch": "x64"
    }
  ]
}
```

### 在目标机上的操作

1. **将应用程序拷贝到目标机**
   - 拷贝Release目录到目标机（例如：`D:\MyApp\Release`）
   - 同时拷贝 `dev_manifest.json`

2. **运行DLL Dependency Checker**
   - 在目标机上启动DLL Dependency Checker
   - 扫描目标机的Release目录（`D:\MyApp\Release`）

3. **加载开发机清单**
   - 点击"加载清单"按钮
   - 选择 `dev_manifest.json`

4. **查看缺失的DLL**
   - 工具会自动对比并显示缺失的DLL
   - 红色标记的就是需要从开发机拷贝的文件

5. **导出拷贝清单**
   - 点击"导出报告"
   - 选择"文本"或"CSV"格式
   - 保存拷贝清单

### 拷贝清单示例
```
=== Copy List Report ===

Copy the following files from dev machine to target machine:

Qt6Core.dll
  Source: C:/Qt/6.5.0/msvc2019_64/bin/Qt6Core.dll

Qt6Widgets.dll
  Source: C:/Qt/6.5.0/msvc2019_64/bin/Qt6Widgets.dll

Qt6Gui.dll
  Source: C:/Qt/6.5.0/msvc2019_64/bin/Qt6Gui.dll
```

6. **根据清单拷贝文件**
   - 回到开发机
   - 根据拷贝清单中的路径，找到对应的DLL
   - 将这些DLL拷贝到目标机的Release目录

7. **验证**
   - 在目标机上再次扫描
   - 确认所有DLL都已存在

## 示例4：导出详细报告

### 场景
你需要生成一份详细的依赖关系报告，用于文档或问题排查。

### 步骤
1. 扫描完成后，点击"导出报告"
2. 选择格式：
   - **文本格式**：适合查看和打印
   - **HTML格式**：适合在浏览器中查看，带颜色标记
   - **CSV格式**：适合在Excel中分析

### HTML报告示例
报告会在浏览器中显示，包含：
- 缺失的DLL列表（红色表格）
- 每个缺失DLL被哪些文件依赖
- 完整的依赖树结构

### CSV报告示例
```csv
Missing DLL,Required By
"CustomLib.dll","MyApp.exe; Helper.dll"
"ThirdParty.dll","MyApp.exe"
```

## 示例5：检测架构不匹配

### 场景
你的应用程序在某些机器上无法启动，怀疑是架构问题。

### 步骤
1. 扫描应用程序目录
2. 查看依赖树中的架构标识
3. 查找橙色标记的 [ARCH MISMATCH] 项

### 问题示例
```
MyApp.exe [x64] v1.0.0.0
  └─ OldLib.dll [x86] v1.0.0.0 [ARCH MISMATCH]  ← 问题！
```

### 解决方案
- 将 `OldLib.dll` 替换为x64版本
- 或者将整个应用程序编译为x86版本

## 示例6：检测循环依赖

### 场景
你的DLL之间可能存在循环依赖，想要确认。

### 步骤
1. 扫描包含DLL的目录
2. 查看依赖树
3. 工具会自动检测并标记循环依赖

### 循环依赖示例
```
LibA.dll [x64]
  └─ LibB.dll [x64]
      └─ LibA.dll [x64] (循环依赖，已停止递归)
```

## 常见问题解决

### Q: 扫描后显示很多系统DLL缺失
A: 工具会自动过滤常见的Windows系统DLL（如kernel32.dll、user32.dll等）。如果显示缺失，可能是：
- 非标准的系统DLL
- 目标系统版本不同
- 需要安装特定的运行时（如VC++ Redistributable）

### Q: 如何知道某个DLL是从哪里加载的
A: 点击依赖树中的DLL，右侧详细信息面板会显示完整路径。

### Q: 扫描速度很慢
A: 
- 避免扫描整个C盘
- 只扫描应用程序目录
- 工具会自动缓存已扫描的DLL

### Q: 导出的清单文件很大
A: 清单包含了所有扫描到的DLL信息。如果只需要特定文件的依赖，使用"扫描文件"功能而不是"扫描文件夹"。

## 高级技巧

### 技巧1：快速定位问题DLL
在依赖树中，使用Ctrl+F搜索特定的DLL名称。

### 技巧2：批量检查多个应用程序
为每个应用程序生成清单文件，然后在目标机器上逐个加载对比。

### 技巧3：版本对比
导出JSON格式的清单，可以使用文本比较工具（如Beyond Compare）对比不同版本的依赖变化。

### 技巧4：自动化部署
将清单文件和拷贝清单集成到自动化部署脚本中，实现自动化的依赖检查和拷贝。

## 总结

DLL Dependency Checker是一个强大的工具，可以帮助你：
- ✅ 快速定位缺失的DLL
- ✅ 了解完整的依赖关系
- ✅ 简化应用程序部署
- ✅ 避免架构不匹配问题
- ✅ 生成详细的依赖报告

掌握这些使用方法，可以大大提高你的开发和部署效率！
