#include "dllcollector.h"
#include "logger.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>

DLLCollector::DLLCollector(QObject *parent)
    : QObject(parent)
{
}

DLLCollector::~DLLCollector()
{
}

DLLCollector::CollectionResult DLLCollector::collectDLLsFromNodes(
    const QList<DependencyScanner::NodePtr>& nodes,
    const QString& targetDirectory,
    ConflictResolution conflictMode)
{
    CollectionResult result;
    result.totalFiles = nodes.size();
    result.successCount = 0;
    result.failedCount = 0;

    // 确保目标目录存在
    QDir targetDir(targetDirectory);
    if (!targetDir.exists()) {
        if (!targetDir.mkpath(".")) {
            // 无法创建目标目录
            for (const auto& node : nodes) {
                if (node) {
                    result.failedFiles[node->fileName] = tr("Failed to create target directory");
                }
            }
            result.failedCount = result.totalFiles;
            emit copyCompleted(result);
            return result;
        }
    }

    // 遍历所有需要复制的DLL节点
    int current = 0;
    for (const auto& node : nodes) {
        current++;

        if (!node) {
            result.failedFiles[tr("Unknown")] = tr("Null dependency node");
            result.failedCount++;
            continue;
        }
        
        QString dllName = node->fileName;
        QString sourcePath = node->filePath;
        QString targetPath = targetDir.filePath(dllName);

        // 发送进度信号
        emit copyProgress(current, result.totalFiles, dllName);

        // 检查源文件是否存在
        if (!node->exists || !QFile::exists(sourcePath)) {
            result.failedFiles[dllName] = tr("Source file not found: %1").arg(sourcePath);
            result.failedCount++;
            continue;
        }

        // 检查目标文件是否已存在
        bool shouldCopy = true;
        if (QFile::exists(targetPath)) {
            if (conflictMode == Skip) {
                shouldCopy = false;
                result.successFiles.append(dllName);
                result.successCount++;
            } else if (conflictMode == Overwrite) {
                // 删除已存在的文件
                if (!QFile::remove(targetPath)) {
                    result.failedFiles[dllName] = tr("Failed to remove existing file");
                    result.failedCount++;
                    shouldCopy = false;
                }
            }
            // AskUser 模式需要在UI层处理
        }

        if (shouldCopy) {
            // 执行复制
            if (copyDLL(sourcePath, targetPath, conflictMode == Overwrite)) {
                result.successFiles.append(dllName);
                result.successCount++;
            } else {
                result.failedFiles[dllName] = tr("Failed to copy file");
                result.failedCount++;
            }
        }
    }

    // 发送完成信号
    emit copyCompleted(result);
    
    return result;
}

bool DLLCollector::copyDLL(const QString& sourcePath, const QString& targetPath, bool overwrite)
{
    // 检查源文件是否存在
    if (!QFile::exists(sourcePath)) {
        LOG_ERROR("DLLCollector", QString("源文件不存在: %1").arg(sourcePath));
        return false;
    }

    // 如果目标文件已存在且需要覆盖，先删除
    if (overwrite && QFile::exists(targetPath)) {
        LOG_DEBUG("DLLCollector", QString("覆盖现有文件: %1").arg(targetPath));
        if (!QFile::remove(targetPath)) {
            LOG_ERROR("DLLCollector", QString("无法删除现有文件: %1").arg(targetPath));
            return false;
        }
    }

    // 执行复制
    bool success = QFile::copy(sourcePath, targetPath);
    if (!success) {
        LOG_ERROR("DLLCollector", QString("文件复制失败: %1 -> %2").arg(sourcePath).arg(targetPath));
    }
    return success;
}
