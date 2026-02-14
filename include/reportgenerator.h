#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include <QString>
#include <QList>
#include <QIODevice>
#include "dependencyscanner.h"
#include "comparisonengine.h"

class ReportGenerator
{
public:
    enum ReportFormat {
        PlainText,
        HTML,
        CSV,
        JSON
    };

    // Generate missing dependency report
    static QString generateMissingReport(const QList<DependencyScanner::DependencyNode*>& roots,
                                        ReportFormat format);

    // Stream missing dependency report to file (for large datasets)
    static bool writeMissingReport(const QList<DependencyScanner::DependencyNode*>& roots,
                                  ReportFormat format,
                                  const QString& filePath);
    static bool writeMissingReportToFile(const QList<DependencyScanner::DependencyNode*>& roots,
                                         ReportFormat format,
                                         QIODevice* device);
    
    // Generate target missing report (only DLL names)
    static QString generateTargetMissingReport(const QList<DependencyScanner::DependencyNode*>& roots,
                                              ReportFormat format = JSON);
    
    // Generate dependency tree report
    static QString generateDependencyTreeReport(DependencyScanner::DependencyNode* root,
                                               ReportFormat format);

private:
    static void collectMissingDependencies(DependencyScanner::DependencyNode* node,
                                          QMap<QString, QStringList>& missingMap);
    static void collectMissingDLLsRecursive(DependencyScanner::DependencyNode* node,
                                           QStringList& missingDLLs);
    static QString generateTreeText(DependencyScanner::DependencyNode* node, int indent);
};

#endif // REPORTGENERATOR_H
