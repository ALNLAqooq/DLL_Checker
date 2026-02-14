#include "comparisonengine.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSysInfo>

ComparisonEngine::MissingReport ComparisonEngine::generateMissingReport(
    const QList<DependencyScanner::DependencyNode*>& roots)
{
    MissingReport report;
    report.generatedTime = QDateTime::currentDateTime();
    report.targetMachine = QSysInfo::machineHostName();
    
    QStringList missingDLLs;
    
    // 递归收集所有缺失的DLL
    for (const auto* root : roots) {
        if (root) {
            collectMissingDLLs(const_cast<DependencyScanner::DependencyNode*>(root), missingDLLs);
        }
    }
    
    // 去重
    missingDLLs.removeDuplicates();
    report.missingDLLs = missingDLLs;
    
    return report;
}

void ComparisonEngine::collectMissingDLLs(
    DependencyScanner::DependencyNode* node,
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
        collectMissingDLLs(child.get(), missingDLLs);
    }
}

bool ComparisonEngine::saveMissingReport(
    const MissingReport& report, 
    const QString& filePath)
{
    QJsonObject root;
    root["generated_time"] = report.generatedTime.toString(Qt::ISODate);
    root["target_machine"] = report.targetMachine;
    
    QJsonArray missingArray;
    for (const QString& dllName : report.missingDLLs) {
        missingArray.append(dllName);
    }
    root["missing_dlls"] = missingArray;
    
    QJsonDocument doc(root);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

ComparisonEngine::MissingReport ComparisonEngine::loadMissingReport(const QString& filePath)
{
    MissingReport report;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return report;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return report;
    }
    
    QJsonObject root = doc.object();
    report.generatedTime = QDateTime::fromString(root["generated_time"].toString(), Qt::ISODate);
    report.targetMachine = root["target_machine"].toString();
    
    QJsonArray missingArray = root["missing_dlls"].toArray();
    for (const QJsonValue& value : missingArray) {
        report.missingDLLs.append(value.toString());
    }
    
    return report;
}

QList<DependencyScanner::DependencyNode*> ComparisonEngine::findMissingDLLsInTree(
    const QList<DependencyScanner::DependencyNode*>& roots,
    const MissingReport& report)
{
    QList<DependencyScanner::DependencyNode*> results;
    
    for (auto* root : roots) {
        if (root) {
            findDLLNodesByName(root, report.missingDLLs, results);
        }
    }
    
    return results;
}

void ComparisonEngine::findDLLNodesByName(
    DependencyScanner::DependencyNode* node,
    const QStringList& dllNames,
    QList<DependencyScanner::DependencyNode*>& results)
{
    if (!node) return;
    
    // 检查当前节点是否匹配
    if (dllNames.contains(node->fileName, Qt::CaseInsensitive)) {
        if (!results.contains(node)) {
            results.append(node);
        }
    }
    
    // 递归处理子节点
    for (const auto& child : node->children) {
        findDLLNodesByName(child.get(), dllNames, results);
    }
}
