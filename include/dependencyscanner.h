#ifndef DEPENDENCYSCANNER_H
#define DEPENDENCYSCANNER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include <QSet>
#include <QMutex>
#include <QSharedPointer>
#include <QWeakPointer>
#include <memory>
#include "peparser.h"

class DependencyScanner : public QObject
{
    Q_OBJECT

public:
    struct DependencyNode {
        QString filePath;
        QString fileName;
        PEParser::Architecture arch;
        QString fileVersion;
        QString productVersion;
        bool exists;
        bool archMismatch;
        QList<QSharedPointer<DependencyNode>> children;
        QWeakPointer<DependencyNode> parent;
        int depth;
        
        DependencyNode() : arch(PEParser::Unknown), exists(false), 
                          archMismatch(false), depth(0) {}
    };

    using NodePtr = QSharedPointer<DependencyNode>;

    explicit DependencyScanner(QObject *parent = nullptr);
    ~DependencyScanner();

    // Scan a single file
    // Returned nodes are shared via QSharedPointer.
    NodePtr scanFile(const QString& filePath, bool includeSystemDLLs = false);
    
    // Scan a directory for all DLL and EXE files
    // Returned nodes are shared via QSharedPointer.
    QList<NodePtr> scanDirectory(const QString& dirPath, bool recursive = false, bool includeSystemDLLs = false);
    
    // Scan a directory with parallel processing
    // Returned nodes are shared via QSharedPointer.
    QList<NodePtr> scanDirectoryParallel(const QString& dirPath, bool recursive = false, bool includeSystemDLLs = false, int threadCount = 4);
    
    // Check for circular dependencies
    bool hasCircularDependency(const NodePtr& node);

    // Clear cache
    void clearCache();

    // Set cancellation flag
    void cancel();

    // Check if cancelled
    bool isCancelled() const;

signals:
    void scanProgress(int current, int total, const QString& currentFile);
    void scanCompleted();

private:
    NodePtr scanFileRecursive(const QString& filePath, const QString& appDir,
                             const NodePtr& parent, int depth, bool includeSystemDLLs);
    NodePtr scanFileWithCustomStack(const QString& filePath, const QString& appDir,
                                   const NodePtr& parent, int depth, bool includeSystemDLLs,
                                   QStringList& customStack, QSet<QString>& customSet);
    QHash<QString, QWeakPointer<DependencyNode>> m_cache;
    QMutex m_cacheMutex;
    QStringList m_scanningStack;
    QSet<QString> m_scanningSet;
    QMutex m_scanningMutex;
    QAtomicInt m_cancelled;
};

Q_DECLARE_METATYPE(DependencyScanner::NodePtr)
Q_DECLARE_METATYPE(QList<DependencyScanner::NodePtr>)

#endif // DEPENDENCYSCANNER_H
