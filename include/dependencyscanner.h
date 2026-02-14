#ifndef DEPENDENCYSCANNER_H
#define DEPENDENCYSCANNER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QSet>
#include <QMutex>
#include <memory>
#include <vector>
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
        std::vector<std::unique_ptr<DependencyNode>> children;
        DependencyNode* parent;
        int depth;
        
        DependencyNode() : arch(PEParser::Unknown), exists(false), 
                          archMismatch(false), parent(nullptr), depth(0) {}
    };

    explicit DependencyScanner(QObject *parent = nullptr);
    ~DependencyScanner();

    // Scan a single file
    DependencyNode* scanFile(const QString& filePath, bool includeSystemDLLs = false);
    
    // Scan a directory for all DLL and EXE files
    QList<DependencyNode*> scanDirectory(const QString& dirPath, bool recursive = false, bool includeSystemDLLs = false);
    
    // Scan a directory with parallel processing
    QList<DependencyNode*> scanDirectoryParallel(const QString& dirPath, bool recursive = false, bool includeSystemDLLs = false, int threadCount = 4);
    
    // Check for circular dependencies
    bool hasCircularDependency(DependencyNode* node);

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
    DependencyNode* scanFileRecursive(const QString& filePath, const QString& appDir,
                                     DependencyNode* parent, int depth, bool includeSystemDLLs);
    DependencyNode* scanFileWithCustomStack(const QString& filePath, const QString& appDir,
                                           DependencyNode* parent, int depth, bool includeSystemDLLs,
                                           QStringList& customStack, QSet<QString>& customSet);
    QMap<QString, DependencyNode*> m_cache;
    QMutex m_cacheMutex;
    QStringList m_scanningStack;
    QSet<QString> m_scanningSet;
    QMutex m_scanningMutex;
    QAtomicInt m_cancelled;
};

Q_DECLARE_METATYPE(DependencyScanner::DependencyNode*)
Q_DECLARE_METATYPE(QList<DependencyScanner::DependencyNode*>)

#endif // DEPENDENCYSCANNER_H
