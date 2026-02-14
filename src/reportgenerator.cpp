#include "reportgenerator.h"
#include "peparser.h"
#include <QMap>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QSysInfo>
#include <QBuffer>
#include <QFile>
#include <QTextStream>

QString ReportGenerator::generateMissingReport(const QList<DependencyScanner::DependencyNode*>& roots,
                                              ReportFormat format)
{
    QByteArray data;
    QBuffer buffer(&data);
    if (!buffer.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return format == HTML ?
            "<html><body><h2>No missing dependencies found!</h2></body></html>" :
            "No missing dependencies found!";
    }

    if (!writeMissingReportToFile(roots, format, &buffer)) {
        return format == HTML ?
            "<html><body><h2>No missing dependencies found!</h2></body></html>" :
            "No missing dependencies found!";
    }

    return QString::fromUtf8(data);
}

bool ReportGenerator::writeMissingReport(const QList<DependencyScanner::DependencyNode*>& roots,
                                         ReportFormat format,
                                         const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    return writeMissingReportToFile(roots, format, &file);
}

bool ReportGenerator::writeMissingReportToFile(const QList<DependencyScanner::DependencyNode*>& roots,
                                               ReportFormat format,
                                               QIODevice* device)
{
    if (!device) {
        return false;
    }

    QTextStream stream(device);

    // Collect all missing dependencies grouped by DLL name
    QMap<QString, QStringList> missingMap;
    for (auto* root : roots) {
        collectMissingDependencies(root, missingMap);
    }

    if (missingMap.isEmpty()) {
        if (format == HTML) {
            stream << "<html><body><h2>No missing dependencies found!</h2></body></html>";
        } else {
            stream << "No missing dependencies found!";
        }
        return true;
    }

    switch (format) {
        case HTML:
            stream << "<html><head><style>"
                   << "body { font-family: Arial, sans-serif; }"
                   << "h2 { color: #d32f2f; }"
                   << "table { border-collapse: collapse; width: 100%; }"
                   << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }"
                   << "th { background-color: #f44336; color: white; }"
                   << "</style></head><body>";
            stream << "<h2>Missing Dependencies Report</h2>";
            stream << "<table><tr><th>Missing DLL</th><th>Required By</th></tr>";
            for (auto it = missingMap.begin(); it != missingMap.end(); ++it) {
                stream << QString("<tr><td><b>%1</b></td><td>%2</td></tr>")
                              .arg(it.key())
                              .arg(it.value().join("<br>"));
            }
            stream << "</table></body></html>";
            break;

        case CSV:
            stream << "Missing DLL,Required By\n";
            for (auto it = missingMap.begin(); it != missingMap.end(); ++it) {
                stream << QString("\"%1\",\"%2\"\n")
                              .arg(it.key())
                              .arg(it.value().join("; "));
            }
            break;

        case JSON: {
            stream << "{\n  \"missing_dependencies\": [\n";
            bool firstEntry = true;
            for (auto it = missingMap.begin(); it != missingMap.end(); ++it) {
                if (!firstEntry) {
                    stream << ",\n";
                }
                firstEntry = false;
                stream << "    {\n";
                stream << QString("      \"dll\": \"%1\",\n").arg(it.key());
                stream << "      \"required_by\": [";
                for (int i = 0; i < it.value().size(); ++i) {
                    stream << QString("\"%1\"").arg(it.value().at(i));
                    if (i < it.value().size() - 1) {
                        stream << ", ";
                    }
                }
                stream << "]\n";
                stream << "    }";
            }
            stream << "\n  ]\n}";
            break;
        }

        default: // PlainText
            stream << "=== Missing Dependencies Report ===\n\n";
            for (auto it = missingMap.begin(); it != missingMap.end(); ++it) {
                stream << QString("Missing DLL: %1\n").arg(it.key());
                stream << "Required by:\n";
                for (const QString& file : it.value()) {
                    stream << QString("  - %1\n").arg(file);
                }
                stream << "\n";
            }
            break;
    }

    return true;
}

QString ReportGenerator::generateTargetMissingReport(const QList<DependencyScanner::DependencyNode*>& roots,
                                                     ReportFormat format)
{
    // 收集所有缺失的DLL
    QStringList missingDLLs;
    for (const auto* root : roots) {
        if (root) {
            collectMissingDLLsRecursive(const_cast<DependencyScanner::DependencyNode*>(root), missingDLLs);
        }
    }
    
    // 去重
    missingDLLs.removeDuplicates();
    
    if (missingDLLs.isEmpty()) {
        return format == HTML ?
            "<html><body><h2>No missing DLLs found!</h2></body></html>" :
            "No missing DLLs found!";
    }
    
    QString report;
    
    switch (format) {
        case JSON: {
            // 生成JSON格式（用于导出缺失报告）
            QJsonObject root;
            root["generated_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            root["target_machine"] = QSysInfo::machineHostName();
            
            QJsonArray missingArray;
            for (const QString& dll : missingDLLs) {
                missingArray.append(dll);
            }
            root["missing_dlls"] = missingArray;
            
            QJsonDocument doc(root);
            report = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
            break;
        }
        
        case HTML:
            report = "<html><head><style>"
                    "body { font-family: Arial, sans-serif; }"
                    "h2 { color: #d32f2f; }"
                    "ul { list-style-type: none; padding-left: 0; }"
                    "li { padding: 5px; border-bottom: 1px solid #eee; }"
                    "</style></head><body>";
            report += "<h2>Missing DLLs Report</h2>";
            report += QString("<p>Total missing: <b>%1</b></p>").arg(missingDLLs.size());
            report += "<ul>";
            
            for (const QString& dll : missingDLLs) {
                report += QString("<li>%1</li>").arg(dll);
            }
            
            report += "</ul></body></html>";
            break;
            
        case CSV:
            report = "Missing DLL\n";
            for (const QString& dll : missingDLLs) {
                report += QString("\"%1\"\n").arg(dll);
            }
            break;
            
        default: // PlainText
            report = "=== Missing DLLs Report ===\n\n";
            report += QString("Total missing: %1\n\n").arg(missingDLLs.size());
            for (const QString& dll : missingDLLs) {
                report += QString("  - %1\n").arg(dll);
            }
            break;
    }
    
    return report;
}

void ReportGenerator::collectMissingDLLsRecursive(DependencyScanner::DependencyNode* node,
                                                  QStringList& missingDLLs)
{
    if (!node) return;
    
    // 如果当前节点是缺失的DLL，添加到列表
    if (!node->exists && !node->fileName.isEmpty()) {
        if (!missingDLLs.contains(node->fileName)) {
            missingDLLs.append(node->fileName);
        }
    }
    
    // 递归处理子节点
    for (const auto& child : node->children) {
        collectMissingDLLsRecursive(child.get(), missingDLLs);
    }
}

QString ReportGenerator::generateDependencyTreeReport(DependencyScanner::DependencyNode* root,
                                                     ReportFormat format)
{
    if (!root) {
        return "No data to report.";
    }
    
    QString report;
    
    switch (format) {
        case HTML:
            report = "<html><head><style>"
                    "body { font-family: 'Courier New', monospace; }"
                    "h2 { color: #4caf50; }"
                    ".missing { color: #f44336; }"
                    ".mismatch { color: #ff9800; }"
                    "</style></head><body>";
            report += "<h2>Dependency Tree Report</h2>";
            report += "<pre>" + generateTreeText(root, 0) + "</pre>";
            report += "</body></html>";
            break;
            
        default: // PlainText
            report = "=== Dependency Tree Report ===\n\n";
            report += generateTreeText(root, 0);
            break;
    }
    
    return report;
}

void ReportGenerator::collectMissingDependencies(DependencyScanner::DependencyNode* node,
                                                QMap<QString, QStringList>& missingMap)
{
    if (!node) return;
    
    // Check children for missing dependencies
    for (const auto& child : node->children) {
        if (!child->exists) {
            QString dllName = child->fileName;
            if (!missingMap.contains(dllName)) {
                missingMap[dllName] = QStringList();
            }
            // Include full path of the file that requires this DLL
            QString requiredByInfo = node->fileName;
            if (!node->filePath.isEmpty()) {
                requiredByInfo += QString(" (%1)").arg(node->filePath);
            }
            if (!missingMap[dllName].contains(requiredByInfo)) {
                missingMap[dllName].append(requiredByInfo);
            }
        }
        
        // Recursively collect from children
        collectMissingDependencies(child.get(), missingMap);
    }
}

QString ReportGenerator::generateTreeText(DependencyScanner::DependencyNode* node, int indent)
{
    if (!node) return QString();
    
    QString result;
    QString indentStr = QString(indent * 2, ' ');
    
    // Node information
    result += indentStr;
    if (indent > 0) {
        result += "└─ ";
    }
    
    result += node->fileName;
    
    // Add status indicators
    if (!node->exists) {
        result += " [MISSING]";
    } else {
        result += QString(" [%1]").arg(PEParser::architectureToString(node->arch));
        if (!node->fileVersion.isEmpty()) {
            result += QString(" v%1").arg(node->fileVersion);
        }
        if (node->archMismatch) {
            result += " [ARCH MISMATCH]";
        }
    }
    
    result += "\n";
    
    // Add children
    for (const auto& child : node->children) {
        result += generateTreeText(child.get(), indent + 1);
    }
    
    return result;
}
