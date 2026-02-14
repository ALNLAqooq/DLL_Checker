#ifndef DLLCOLLECTOR_H
#define DLLCOLLECTOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include "dependencyscanner.h"

class DLLCollector : public QObject
{
    Q_OBJECT

public:
    // 收集结果结构
    struct CollectionResult {
        int totalFiles;
        int successCount;
        int failedCount;
        QStringList successFiles;
        QMap<QString, QString> failedFiles;  // 文件名 -> 错误信息
    };

    // 冲突解决策略
    enum ConflictResolution {
        Overwrite,      // 覆盖已存在的文件
        Skip,           // 跳过已存在的文件
        AskUser         // 每次询问用户
    };

    explicit DLLCollector(QObject *parent = nullptr);
    ~DLLCollector();

    // 从高亮节点列表收集DLL（新方法）
    CollectionResult collectDLLsFromNodes(
        const QList<DependencyScanner::DependencyNode*>& nodes,
        const QString& targetDirectory,
        ConflictResolution conflictMode = AskUser
    );

    // 复制单个DLL（静态方法）
    static bool copyDLL(
        const QString& sourcePath,
        const QString& targetPath,
        bool overwrite = false
    );

signals:
    void copyProgress(int current, int total, const QString& currentFile);
    void copyCompleted(const CollectionResult& result);
};

#endif // DLLCOLLECTOR_H
