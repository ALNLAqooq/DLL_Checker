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
    static QString generateMissingReport(const QList<DependencyScanner::NodePtr>& roots,
                                        ReportFormat format);

    // Stream missing dependency report to file (for large datasets)
    static bool writeMissingReport(const QList<DependencyScanner::NodePtr>& roots,
                                  ReportFormat format,
                                  const QString& filePath);
    static bool writeMissingReportToFile(const QList<DependencyScanner::NodePtr>& roots,
                                         ReportFormat format,
                                         QIODevice* device);
    
    // Generate target missing report (only DLL names)
    static QString generateTargetMissingReport(const QList<DependencyScanner::NodePtr>& roots,
                                              ReportFormat format = JSON);
    
    // Generate dependency tree report
    static QString generateDependencyTreeReport(const DependencyScanner::NodePtr& root,
                                               ReportFormat format);

private:
    static void collectMissingDependencies(const DependencyScanner::NodePtr& node,
                                          QMap<QString, QStringList>& missingMap);
    static void collectMissingDLLsRecursive(const DependencyScanner::NodePtr& node,
                                           QStringList& missingDLLs);
    static QString generateTreeText(const DependencyScanner::NodePtr& node, int indent);
};

#endif // REPORTGENERATOR_H
