#ifndef COMPARISONENGINE_H
#define COMPARISONENGINE_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QList>
#include "dependencyscanner.h"

class ComparisonEngine
{
public:
    // 缺失报告结构（目标机导出）
    struct MissingReport {
        QStringList missingDLLs;    // 缺失的DLL名称列表
        QDateTime generatedTime;
        QString targetMachine;
    };

    // 完整依赖链结构
    struct DependencyChain {
        QString dllName;
        QStringList chain;           // 完整依赖链，从根节点到该DLL
        PEParser::Architecture arch;
        QString systemPath;          // 如果在系统目录找到
    };

    // 增强的缺失报告（包含依赖链）
    struct EnhancedMissingReport {
        QList<DependencyChain> dependencyChains;
        QStringList msvcRuntimeDLLs; // 需要的MSVC运行时库
        QStringList architectureMismatches; // 架构不匹配的DLL
        QDateTime generatedTime;
        QString targetMachine;
        QString sourceMachine;      // 开发机（如果有）
    };

    // 从目标机扫描结果生成缺失报告
    static MissingReport generateMissingReport(
        const QList<DependencyScanner::NodePtr>& roots
    );

    // 保存缺失报告到文件
    static bool saveMissingReport(
        const MissingReport& report,
        const QString& filePath
    );

    // 加载缺失报告
    static MissingReport loadMissingReport(const QString& filePath);

    // 生成增强的缺失报告（包含完整依赖链）
    static EnhancedMissingReport generateEnhancedMissingReport(
        const QList<DependencyScanner::NodePtr>& roots
    );

    // 保存增强报告到文件
    static bool saveEnhancedMissingReport(
        const EnhancedMissingReport& report,
        const QString& filePath
    );

    // 加载增强报告
    static EnhancedMissingReport loadEnhancedMissingReport(const QString& filePath);

    // 在依赖树中查找并标记缺失的DLL
    static QList<DependencyScanner::NodePtr> findMissingDLLsInTree(
        const QList<DependencyScanner::NodePtr>& roots,
        const MissingReport& report
    );

private:
    // 递归查找DLL节点
    static void findDLLNodesByName(
        const DependencyScanner::NodePtr& node,
        const QStringList& dllNames,
        QList<DependencyScanner::NodePtr>& results
    );

    // 递归收集缺失的DLL
    static void collectMissingDLLs(
        const DependencyScanner::NodePtr& node,
        QStringList& missingDLLs
    );

    // 收集增强信息（依赖链、MSVC运行时、架构不匹配）
    static void collectEnhancedInfo(
        const DependencyScanner::NodePtr& node,
        const QStringList& currentChain,
        QList<DependencyChain>& dependencyChains,
        QSet<QString>& missingDLLs,
        QSet<QString>& msvcDLLs,
        QSet<QString>& archMismatches,
        const QStringList& systemPaths
    );
};

#endif // COMPARISONENGINE_H
