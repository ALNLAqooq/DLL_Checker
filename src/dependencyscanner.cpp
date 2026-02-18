#include "dependencyscanner.h"
#include "peparser.h"
#include "pathresolver.h"
#include "logger.h"
#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <QFuture>
#include <QtConcurrent>
#include <QtConcurrentMap>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaObject>
#include <utility>

DependencyScanner::DependencyScanner(QObject *parent)
    : QObject(parent)
    , m_cancelled(0)
{
}

DependencyScanner::~DependencyScanner()
{
    clearCache();
}

static DependencyScanner::NodePtr cloneNodeShallow(
    const DependencyScanner::DependencyNode* src,
    const DependencyScanner::NodePtr& parent,
    int depth)
{
    if (!src) {
        return DependencyScanner::NodePtr();
    }

    DependencyScanner::NodePtr node(new DependencyScanner::DependencyNode());
    node->filePath = src->filePath;
    node->fileName = src->fileName;
    node->arch = src->arch;
    node->fileVersion = src->fileVersion;
    node->productVersion = src->productVersion;
    node->exists = src->exists;
    node->archMismatch = src->archMismatch;
    node->parent = parent;
    node->depth = depth;

    return node;
}

static DependencyScanner::NodePtr cloneNodeDeep(
    const DependencyScanner::DependencyNode* src,
    const DependencyScanner::NodePtr& parent,
    int depth)
{
    if (!src) {
        return DependencyScanner::NodePtr();
    }

    DependencyScanner::NodePtr node = cloneNodeShallow(src, parent, depth);
    if (!node) {
        return DependencyScanner::NodePtr();
    }

    for (const auto& child : src->children) {
        if (!child) {
            continue;
        }
        DependencyScanner::NodePtr childClone =
            cloneNodeDeep(child.data(), node, depth + 1);
        if (childClone) {
            node->children.append(childClone);
        }
    }

    return node;
}
DependencyScanner::NodePtr DependencyScanner::scanFile(const QString& filePath, bool includeSystemDLLs)
{
    clearCache();
    m_scanningStack.clear();
    m_scanningSet.clear();
    
    QFileInfo fileInfo(filePath);
    QString appDir = fileInfo.absolutePath();
    
    return scanFileRecursive(filePath, appDir, NodePtr(), 0, includeSystemDLLs);
}

QList<DependencyScanner::NodePtr> DependencyScanner::scanDirectory(const QString& dirPath, bool recursive, bool includeSystemDLLs)
{
    QList<NodePtr> results;
    clearCache();
    m_scanningStack.clear();
    m_scanningSet.clear();
    
    // Find all DLL and EXE files
    QStringList filters;
    filters << "*.dll" << "*.exe";
    
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive) {
        flags = QDirIterator::Subdirectories;
    }
    
    QStringList files;
    QDirIterator it(dirPath, filters, QDir::Files, flags);
    while (it.hasNext()) {
        if (isCancelled()) {
            break;
        }
        files.append(it.next());
    }
    
    int total = files.size();
    int current = 0;

    for (const QString& filePath : files) {
        if (isCancelled()) {
            break;
        }

        current++;
        emit scanProgress(current, total, QFileInfo(filePath).fileName());

        m_scanningStack.clear();
        m_scanningSet.clear();

        const QString appDir = QFileInfo(filePath).absolutePath();
        NodePtr node = scanFileRecursive(filePath, appDir, NodePtr(), 0, includeSystemDLLs);
        if (node) {
            results.append(node);
        }
    }

    emit scanCompleted();
    return results;
}

QList<DependencyScanner::NodePtr> DependencyScanner::scanDirectoryParallel(const QString& dirPath, bool recursive, bool includeSystemDLLs, int threadCount)
{
    LOG_INFO("DependencyScanner", QString("开始并行扫描目录: %1 (线程数: %2)").arg(dirPath).arg(threadCount));
    
    QList<NodePtr> results;
    clearCache();
    m_scanningStack.clear();
    m_scanningSet.clear();
    
    // Find all DLL and EXE files
    QStringList filters;
    filters << "*.dll" << "*.exe";
    
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive) {
        flags = QDirIterator::Subdirectories;
    }
    
    QStringList files;
    QDirIterator it(dirPath, filters, QDir::Files, flags);
    while (it.hasNext()) {
        if (isCancelled()) {
            break;
        }
        files.append(it.next());
    }
    
    LOG_INFO("DependencyScanner", QString("找到 %1 个文件待扫描，使用 %2 个线程并行处理")
        .arg(files.size()).arg(threadCount));
    
    QThreadPool* pool = QThreadPool::globalInstance();
    int originalMaxThreadCount = pool->maxThreadCount();
    pool->setMaxThreadCount(threadCount);
    
    QMutex resultMutex;
    int total = files.size();
    QAtomicInt current(0);
    
    class ScanTask : public QRunnable {
    public:
        ScanTask(const QString& filePath, DependencyScanner* scanner, 
                 bool includeSystemDLLs, QAtomicInt* current, int total,
                 QMutex* resultMutex, QList<NodePtr>* results)
            : m_filePath(filePath), m_scanner(scanner), 
              m_includeSystemDLLs(includeSystemDLLs), 
              m_current(current), m_total(total),
              m_resultMutex(resultMutex), m_results(results) {}
              
        void run() override {
            if (m_scanner->isCancelled()) {
                return;
            }
            
            int currentVal = m_current->fetchAndAddAcquire(1) + 1;
            QMetaObject::invokeMethod(m_scanner, "scanProgress", 
                Qt::QueuedConnection,
                Q_ARG(int, currentVal), 
                Q_ARG(int, m_total), 
                Q_ARG(QString, QFileInfo(m_filePath).fileName()));
            
            QStringList threadStack;
            QSet<QString> threadSet;
            QString appDir = QFileInfo(m_filePath).absolutePath();
            
            NodePtr node = m_scanner->scanFileWithCustomStack(
                m_filePath, appDir, NodePtr(), 0, m_includeSystemDLLs, threadStack, threadSet);
            
            if (node) {
                QMutexLocker locker(m_resultMutex);
                m_results->append(node);
            }
        }
        
    private:
        QString m_filePath;
        DependencyScanner* m_scanner;
        bool m_includeSystemDLLs;
        QAtomicInt* m_current;
        int m_total;
        QMutex* m_resultMutex;
        QList<NodePtr>* m_results;
    };
    
    QList<ScanTask*> tasks;
    for (const QString& filePath : files) {
        ScanTask* task = new ScanTask(filePath, this, includeSystemDLLs, 
                                       &current, total, &resultMutex, &results);
        task->setAutoDelete(true);
        tasks.append(task);
        pool->start(task);
    }
    
    pool->waitForDone();
    pool->setMaxThreadCount(originalMaxThreadCount);
    
    if (isCancelled()) {
        LOG_WARNING("DependencyScanner", "并行扫描被用户取消");
    }
    
    LOG_INFO("DependencyScanner", QString("并行扫描完成，共扫描 %1 个文件").arg(results.size()));
    emit scanCompleted();
    return results;
}

DependencyScanner::NodePtr DependencyScanner::scanFileWithCustomStack(
    const QString& filePath,
    const QString& appDir,
    const DependencyScanner::NodePtr& parent,
    int depth,
    bool includeSystemDLLs,
    QStringList& customStack,
    QSet<QString>& customSet)
{
    // Check for cancellation
    if (isCancelled()) {
        return NodePtr();
    }

    // Limit recursion depth to prevent stack overflow
    const int MAX_DEPTH = 50;
    if (depth > MAX_DEPTH) {
        LOG_DEBUG("DependencyScanner", QString("达到最大递归深度: %1").arg(filePath));
        return NodePtr();
    }

    // Check for circular dependency
    QString lowerPath = filePath.toLower();
    if (customSet.contains(lowerPath)) {
        LOG_DEBUG("DependencyScanner", QString("检测到循环依赖: %1").arg(filePath));
        NodePtr node(new DependencyNode());
        node->filePath = filePath;
        node->fileName = QFileInfo(filePath).fileName();
        node->exists = true;
        node->parent = parent;
        node->depth = depth;
        return node;
    }

    // Check cache (thread-safe)
    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_cache.contains(lowerPath)) {
            QSharedPointer<DependencyNode> cached = m_cache.value(lowerPath).toStrongRef();
            if (cached) {
                return cloneNodeDeep(cached.data(), parent, depth);
            }
        }
    }

    // Add to custom scanning stack
    customStack.append(lowerPath);
    customSet.insert(lowerPath);

    auto popCurrent = [&customStack, &customSet]() {
        if (!customStack.isEmpty()) {
            const QString current = customStack.takeLast();
            customSet.remove(current);
        }
    };

    // Create node
    NodePtr node(new DependencyNode());
    node->filePath = filePath;
    node->fileName = QFileInfo(filePath).fileName();
    node->parent = parent;
    node->depth = depth;

    // Check if file exists
    QFileInfo fileInfo(filePath);
    node->exists = fileInfo.exists();

    if (!node->exists) {
        popCurrent();
        return node;
    }
    
    // Parse PE file
    PEParser::PEInfo peInfo = PEParser::parsePEFile(filePath);
    if (!peInfo.isValid) {
        LOG_DEBUG("DependencyScanner", QString("PE解析失败: %1").arg(filePath));
        popCurrent();
        return node;
    }
    
    node->arch = peInfo.arch;
    node->fileVersion = peInfo.fileVersion;
    node->productVersion = peInfo.productVersion;
    
    // Check architecture mismatch with parent
    if (parent && parent->arch != PEParser::Unknown && node->arch != PEParser::Unknown) {
        if (parent->arch == PEParser::x64 && node->arch == PEParser::x86) {
            node->archMismatch = true;
            LOG_DEBUG("DependencyScanner", QString("架构不匹配: %1 (x64) -> %2 (x86)")
                .arg(parent->fileName)
                .arg(node->fileName));
        }
    }
    
    // Scan dependencies
    for (const QString& dllName : peInfo.dependencies) {
        if (isCancelled()) {
            break;
        }

        // Skip system DLLs to reduce noise (unless user wants to see them)
        if (!includeSystemDLLs && PathResolver::isSystemDLL(dllName)) {
            continue;
        }
        
        // Resolve DLL path
        PathResolver::ResolveResult resolveResult = PathResolver::resolveDLLPath(dllName, appDir);
        
        NodePtr childNode;

        if (resolveResult.found) {
            // Recursively scan the dependency
            childNode = scanFileWithCustomStack(resolveResult.foundPath, appDir, node, depth + 1, includeSystemDLLs, customStack, customSet);
        } else {
            // DLL not found
            childNode = NodePtr(new DependencyNode());
            childNode->filePath = dllName;
            childNode->fileName = dllName;
            childNode->exists = false;
            childNode->parent = node;
            childNode->depth = depth + 1;
        }
        
        if (childNode) {
            node->children.append(childNode);
        }
    }
    
    // Cache the node (thread-safe)
    {
        QMutexLocker locker(&m_cacheMutex);
        m_cache.insert(lowerPath, QWeakPointer<DependencyNode>(node));
    }
    
    // Remove from scanning stack
    popCurrent();
    
    return node;
}

bool DependencyScanner::hasCircularDependency(const DependencyScanner::NodePtr& node)
{
    if (!node) return false;
    
    // Check if this node's file path is in the scanning set
    return m_scanningSet.contains(node->filePath.toLower());
}

void DependencyScanner::clearCache()
{
    m_cancelled.storeRelease(0);
    m_scanningStack.clear();
    m_scanningSet.clear();
    m_cache.clear();
}

void DependencyScanner::cancel()
{
    m_cancelled.storeRelease(1);
}

bool DependencyScanner::isCancelled() const
{
    return m_cancelled.loadAcquire() != 0;
}

DependencyScanner::NodePtr DependencyScanner::scanFileRecursive(const QString& filePath,
                                                                        const QString& appDir,
                                                                        const DependencyScanner::NodePtr& parent,
                                                                        int depth,
                                                                        bool includeSystemDLLs)
{
    // Check for cancellation
    if (isCancelled()) {
        return NodePtr();
    }

    // Limit recursion depth to prevent stack overflow
    const int MAX_DEPTH = 50;
    if (depth > MAX_DEPTH) {
        LOG_WARNING("DependencyScanner", QString("达到最大递归深度: %1").arg(filePath));
        return NodePtr();
    }

    // Check for circular dependency
    QString lowerPath = filePath.toLower();
    if (m_scanningSet.contains(lowerPath)) {
        // Circular dependency detected
        LOG_WARNING("DependencyScanner", QString("检测到循环依赖: %1").arg(filePath));
        NodePtr node(new DependencyNode());
        node->filePath = filePath;
        node->fileName = QFileInfo(filePath).fileName();
        node->exists = true;
        node->parent = parent;
        node->depth = depth;
        return node;
    }

    // Check cache
    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_cache.contains(lowerPath)) {
            // Fast path: avoid deep subtree cloning on repeated dependencies.
            // This keeps scan latency stable for large dependency graphs.
            QSharedPointer<DependencyNode> cached = m_cache.value(lowerPath).toStrongRef();
            if (cached) {
                return cloneNodeDeep(cached.data(), parent, depth);
            }
        }
    }

    // Add to scanning stack
    m_scanningStack.append(lowerPath);
    m_scanningSet.insert(lowerPath);

    auto popCurrent = [this]() {
        if (!m_scanningStack.isEmpty()) {
            const QString current = m_scanningStack.takeLast();
            m_scanningSet.remove(current);
        }
    };

    // Create node
    NodePtr node(new DependencyNode());
    node->filePath = filePath;
    node->fileName = QFileInfo(filePath).fileName();
    node->parent = parent;
    node->depth = depth;

    // Check if file exists
    QFileInfo fileInfo(filePath);
    node->exists = fileInfo.exists();

    if (!node->exists) {
        popCurrent();
        return node;
    }
    
    // Parse PE file
    PEParser::PEInfo peInfo = PEParser::parsePEFile(filePath);
    if (!peInfo.isValid) {
        LOG_ERROR("DependencyScanner", QString("PE解析失败: %1").arg(filePath));
        popCurrent();
        return node;
    }
    
    node->arch = peInfo.arch;
    node->fileVersion = peInfo.fileVersion;
    node->productVersion = peInfo.productVersion;
    
    LOG_DEBUG("DependencyScanner", QString("解析成功: %1, 架构: %2, 依赖数: %3")
        .arg(node->fileName)
        .arg(PEParser::architectureToString(node->arch))
        .arg(peInfo.dependencies.size()));
    
    // Check architecture mismatch with parent
    if (parent && parent->arch != PEParser::Unknown && node->arch != PEParser::Unknown) {
        // x64 cannot load x86 DLLs (but x86 can load x86 DLLs on x64 system via WOW64)
        if (parent->arch == PEParser::x64 && node->arch == PEParser::x86) {
            node->archMismatch = true;
            LOG_WARNING("DependencyScanner", QString("架构不匹配: %1 (x64) -> %2 (x86)")
                .arg(parent->fileName)
                .arg(node->fileName));
        }
    }
    
    // Scan dependencies
    for (const QString& dllName : peInfo.dependencies) {
        if (isCancelled()) {
            break;
        }

        // Skip system DLLs to reduce noise (unless user wants to see them)
        if (!includeSystemDLLs && PathResolver::isSystemDLL(dllName)) {
            continue;
        }
        
        // Resolve DLL path
        PathResolver::ResolveResult resolveResult = PathResolver::resolveDLLPath(dllName, appDir);
        
        NodePtr childNode;

        if (resolveResult.found) {
            // Recursively scan the dependency
            childNode = scanFileRecursive(resolveResult.foundPath, appDir, node, depth + 1, includeSystemDLLs);
        } else {
            // DLL not found
            childNode = NodePtr(new DependencyNode());
            childNode->filePath = dllName;
            childNode->fileName = dllName;
            childNode->exists = false;
            childNode->parent = node;
            childNode->depth = depth + 1;
        }
        
        if (childNode) {
            node->children.append(childNode);
        }
    }
    
    // Cache the node
    {
        QMutexLocker locker(&m_cacheMutex);
        m_cache.insert(lowerPath, QWeakPointer<DependencyNode>(node));
    }
    
    // Remove from scanning stack
    popCurrent();
    
    return node;
}
