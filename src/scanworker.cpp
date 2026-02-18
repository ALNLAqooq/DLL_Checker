#include "scanworker.h"
#include "logger.h"
#include <QFileInfo>

ScanWorker::ScanWorker(QObject *parent)
    : QObject(parent)
    , m_scanner(new DependencyScanner(this))
    , m_cancelled(0)
{
    connect(m_scanner, &DependencyScanner::scanProgress,
            this, &ScanWorker::onScanProgress);
}

ScanWorker::~ScanWorker()
{
    // Results are shared via QSharedPointer, no manual deletion required.
    m_results.clear();
}

void ScanWorker::scanFile(const QString& filePath, bool includeSystemDLLs)
{
    m_cancelled.storeRelease(0);
    if (m_cancelled.loadAcquire()) {
        emit scanError(tr("扫描已取消"));
        return;
    }

    m_scanner->clearCache();
    emit scanProgress(0, 1, QFileInfo(filePath).fileName());

    DependencyScanner::NodePtr node = m_scanner->scanFile(filePath, includeSystemDLLs);
    if (node) {
        m_results.clear();
        m_results.append(node);
        emit scanProgress(1, 1, QFileInfo(filePath).fileName());
        emit scanFinished(m_results);
    } else {
        QFileInfo fileInfo(filePath);
        QString errorMsg = QString("无法扫描文件: %1\n\n"
            "文件信息：\n"
            "- 文件名: %2\n"
            "- 文件大小: %3 字节\n\n"
            "可能的原因：\n"
            "1. 文件格式不正确（应为.exe或.dll）\n"
            "2. 文件可能已损坏\n"
            "3. 没有足够的访问权限\n"
            "4. 文件可能被其他程序锁定")
            .arg(filePath)
            .arg(fileInfo.fileName())
            .arg(fileInfo.size());
        emit scanError(errorMsg);
    }
}

void ScanWorker::scanDirectory(const QString& dirPath, bool recursive, bool includeSystemDLLs)
{
    LOG_INFO("ScanWorker", QString("启动目录扫描: %1 (递归: %2, 包含系统DLL: %3)")
        .arg(dirPath).arg(recursive).arg(includeSystemDLLs));
    
    m_cancelled.storeRelease(0);
    if (m_cancelled.loadAcquire()) {
        LOG_WARNING("ScanWorker", "扫描已被取消");
        emit scanError(tr("扫描已取消"));
        return;
    }

    m_scanner->clearCache();

    if (m_cancelled.loadAcquire()) {
        LOG_WARNING("ScanWorker", "扫描已被取消");
        emit scanError(tr("扫描已取消"));
        return;
    }

    m_results.clear();
    m_results = m_scanner->scanDirectory(dirPath, recursive, includeSystemDLLs);
    LOG_INFO("ScanWorker", "目录扫描完成");
    emit scanFinished(m_results);
}

void ScanWorker::scanDirectoryParallel(const QString& dirPath, bool recursive, bool includeSystemDLLs, int threadCount)
{
    LOG_INFO("ScanWorker", QString("启动并行目录扫描: %1 (递归: %2, 包含系统DLL: %3, 线程数: %4)")
        .arg(dirPath).arg(recursive).arg(includeSystemDLLs).arg(threadCount));
    
    m_cancelled.storeRelease(0);
    if (m_cancelled.loadAcquire()) {
        LOG_WARNING("ScanWorker", "扫描已被取消");
        emit scanError(tr("扫描已取消"));
        return;
    }

    m_scanner->clearCache();

    if (m_cancelled.loadAcquire()) {
        LOG_WARNING("ScanWorker", "扫描已被取消");
        emit scanError(tr("扫描已取消"));
        return;
    }

    m_results.clear();
    m_results = m_scanner->scanDirectoryParallel(dirPath, recursive, includeSystemDLLs, threadCount);
    LOG_INFO("ScanWorker", "并行目录扫描完成");
    emit scanFinished(m_results);
}

void ScanWorker::cancel()
{
    LOG_INFO("ScanWorker", "收到取消扫描请求");
    m_cancelled.storeRelease(1);
    m_scanner->cancel();
}

bool ScanWorker::isCancelled() const
{
    return m_cancelled.loadAcquire() != 0;
}

void ScanWorker::onScanProgress(int current, int total, const QString& currentFile)
{
    if (m_cancelled.loadAcquire()) {
        return;
    }

    emit scanProgress(current, total, currentFile);
}
