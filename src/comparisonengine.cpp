#include "comparisonengine.h"
#include "pathresolver.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSysInfo>
#include <QSet>

ComparisonEngine::MissingReport ComparisonEngine::generateMissingReport(
    const QList<DependencyScanner::NodePtr>& roots)
{
    MissingReport report;
    report.generatedTime = QDateTime::currentDateTime();
    report.targetMachine = QSysInfo::machineHostName();
    
    QStringList missingDLLs;
    
    // 递归收集所有缺失的DLL
    for (const auto& root : roots) {
        if (root) {
            collectMissingDLLs(root, missingDLLs);
        }
    }
    
    // 去重
    missingDLLs.removeDuplicates();
    report.missingDLLs = missingDLLs;
    
    return report;
}

void ComparisonEngine::collectMissingDLLs(
    const DependencyScanner::NodePtr& node,
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
        collectMissingDLLs(child, missingDLLs);
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

QList<DependencyScanner::NodePtr> ComparisonEngine::findMissingDLLsInTree(
    const QList<DependencyScanner::NodePtr>& roots,
    const MissingReport& report)
{
    QList<DependencyScanner::NodePtr> results;
    
    for (const auto& root : roots) {
        if (root) {
            findDLLNodesByName(root, report.missingDLLs, results);
        }
    }
    
    return results;
}

void ComparisonEngine::findDLLNodesByName(
    const DependencyScanner::NodePtr& node,
    const QStringList& dllNames,
    QList<DependencyScanner::NodePtr>& results)
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
        findDLLNodesByName(child, dllNames, results);
    }
}

ComparisonEngine::EnhancedMissingReport ComparisonEngine::generateEnhancedMissingReport(
    const QList<DependencyScanner::NodePtr>& roots)
{
    EnhancedMissingReport report;
    report.generatedTime = QDateTime::currentDateTime();
    report.targetMachine = QSysInfo::machineHostName();
    report.sourceMachine = report.targetMachine;

    QSet<QString> missingDLLs;
    QSet<QString> msvcDLLs;
    QSet<QString> archMismatches;

    QStringList systemPaths = PathResolver::getSystemSearchPaths();

    for (const auto& root : roots) {
        if (!root) continue;

        collectEnhancedInfo(root, QStringList(), report.dependencyChains,
                           missingDLLs, msvcDLLs, archMismatches, systemPaths);
    }

    report.msvcRuntimeDLLs = msvcDLLs.values();
    report.architectureMismatches = archMismatches.values();

    return report;
}

void ComparisonEngine::collectEnhancedInfo(
    const DependencyScanner::NodePtr& node,
    const QStringList& currentChain,
    QList<DependencyChain>& dependencyChains,
    QSet<QString>& missingDLLs,
    QSet<QString>& msvcDLLs,
    QSet<QString>& archMismatches,
    const QStringList& systemPaths)
{
    if (!node) return;

    QStringList chain = currentChain;
    chain.append(node->fileName);

    if (!node->exists && !node->fileName.isEmpty()) {
        QString lowerName = node->fileName.toLower();
        if (!missingDLLs.contains(lowerName)) {
            missingDLLs.insert(lowerName);

            DependencyChain depChain;
            depChain.dllName = node->fileName;
            depChain.chain = chain;
            depChain.arch = node->arch;

            for (const QString& sysPath : systemPaths) {
                QString fullPath = QDir(sysPath).filePath(node->fileName);
                if (QFile::exists(fullPath)) {
                    depChain.systemPath = fullPath;
                    break;
                }
            }

            dependencyChains.append(depChain);
        }
    }

    if (node->archMismatch && !node->fileName.isEmpty()) {
        archMismatches.insert(node->fileName);
    }

    QString lowerName = node->fileName.toLower();
    if (lowerName.contains("msvcp") || lowerName.contains("vcruntime") ||
        lowerName.contains("concrt") || lowerName.contains("api-ms-win-crt")) {
        msvcDLLs.insert(node->fileName);
    }

    for (const auto& child : node->children) {
        collectEnhancedInfo(child, chain, dependencyChains,
                          missingDLLs, msvcDLLs, archMismatches, systemPaths);
    }
}

bool ComparisonEngine::saveEnhancedMissingReport(
    const EnhancedMissingReport& report,
    const QString& filePath)
{
    QJsonObject root;
    root["generated_time"] = report.generatedTime.toString(Qt::ISODate);
    root["target_machine"] = report.targetMachine;
    root["source_machine"] = report.sourceMachine;

    QJsonArray chainsArray;
    for (const DependencyChain& chain : report.dependencyChains) {
        QJsonObject chainObj;
        chainObj["dll_name"] = chain.dllName;

        QJsonArray chainPathArray;
        for (const QString& path : chain.chain) {
            chainPathArray.append(path);
        }
        chainObj["chain"] = chainPathArray;

        chainObj["arch"] = PEParser::architectureToString(chain.arch);
        chainObj["system_path"] = chain.systemPath;
        chainsArray.append(chainObj);
    }
    root["dependency_chains"] = chainsArray;

    QJsonArray msvcArray;
    for (const QString& dll : report.msvcRuntimeDLLs) {
        msvcArray.append(dll);
    }
    root["msvc_runtime_dlls"] = msvcArray;

    QJsonArray archArray;
    for (const QString& dll : report.architectureMismatches) {
        archArray.append(dll);
    }
    root["architecture_mismatches"] = archArray;

    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

ComparisonEngine::EnhancedMissingReport ComparisonEngine::loadEnhancedMissingReport(const QString& filePath)
{
    EnhancedMissingReport report;

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
    report.sourceMachine = root["source_machine"].toString();

    QJsonArray chainsArray = root["dependency_chains"].toArray();
    for (const QJsonValue& value : chainsArray) {
        QJsonObject chainObj = value.toObject();
        DependencyChain chain;
        chain.dllName = chainObj["dll_name"].toString();

        QJsonArray chainPathArray = chainObj["chain"].toArray();
        for (const QJsonValue& pathValue : chainPathArray) {
            chain.chain.append(pathValue.toString());
        }

        QString archStr = chainObj["arch"].toString();
        if (archStr == "x86") {
            chain.arch = PEParser::x86;
        } else if (archStr == "x64") {
            chain.arch = PEParser::x64;
        } else {
            chain.arch = PEParser::Unknown;
        }

        chain.systemPath = chainObj["system_path"].toString();
        report.dependencyChains.append(chain);
    }

    QJsonArray msvcArray = root["msvc_runtime_dlls"].toArray();
    for (const QJsonValue& value : msvcArray) {
        report.msvcRuntimeDLLs.append(value.toString());
    }

    QJsonArray archArray = root["architecture_mismatches"].toArray();
    for (const QJsonValue& value : archArray) {
        report.architectureMismatches.append(value.toString());
    }

    return report;
}
