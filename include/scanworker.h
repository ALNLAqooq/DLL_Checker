#ifndef SCANWORKER_H
#define SCANWORKER_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QList>
#include <QAtomicInt>
#include "dependencyscanner.h"

class ScanWorker : public QObject
{
    Q_OBJECT

public:
    enum ScanType {
        ScanFile,
        ScanDirectory
    };

    struct ScanParameters {
        ScanType type;
        QString path;
        bool recursive;
        bool includeSystemDLLs;

        ScanParameters() : type(ScanFile), recursive(false), includeSystemDLLs(false) {}
    };

    explicit ScanWorker(QObject *parent = nullptr);
    ~ScanWorker();

    void scanFile(const QString& filePath, bool includeSystemDLLs = false);
    void scanDirectory(const QString& dirPath, bool recursive = false, bool includeSystemDLLs = false);
    void scanDirectoryParallel(const QString& dirPath, bool recursive = false, bool includeSystemDLLs = false, int threadCount = 4);
    void cancel();

    bool isCancelled() const;

signals:
    void scanProgress(int current, int total, const QString& currentFile);
    void scanFinished(QList<DependencyScanner::DependencyNode*> results);
    void scanError(const QString& errorMessage);

private slots:
    void onScanProgress(int current, int total, const QString& currentFile);
    void onScanCompleted();

private:
    DependencyScanner* m_scanner;
    QAtomicInt m_cancelled;
    QList<DependencyScanner::DependencyNode*> m_results;
};

#endif // SCANWORKER_H
