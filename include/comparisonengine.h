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

    // 从目标机扫描结果生成缺失报告
    static MissingReport generateMissingReport(
        const QList<DependencyScanner::DependencyNode*>& roots
    );
    
    // 保存缺失报告到文件
    static bool saveMissingReport(
        const MissingReport& report, 
        const QString& filePath
    );
    
    // 加载缺失报告
    static MissingReport loadMissingReport(const QString& filePath);
    
    // 在依赖树中查找并标记缺失的DLL
    static QList<DependencyScanner::DependencyNode*> findMissingDLLsInTree(
        const QList<DependencyScanner::DependencyNode*>& roots,
        const MissingReport& report
    );

private:
    // 递归查找DLL节点
    static void findDLLNodesByName(
        DependencyScanner::DependencyNode* node,
        const QStringList& dllNames,
        QList<DependencyScanner::DependencyNode*>& results
    );
    
    // 递归收集缺失的DLL
    static void collectMissingDLLs(
        DependencyScanner::DependencyNode* node,
        QStringList& missingDLLs
    );
};

#endif // COMPARISONENGINE_H
