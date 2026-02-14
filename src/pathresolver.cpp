#include "pathresolver.h"
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <Windows.h>

namespace {
QStringList cachedSystemPaths()
{
    static QMutex mutex;
    static bool initialized = false;
    static QStringList paths;

    QMutexLocker locker(&mutex);
    if (initialized) {
        return paths;
    }

    wchar_t system32Path[MAX_PATH];
    if (GetSystemDirectoryW(system32Path, MAX_PATH)) {
        paths.append(QString::fromWCharArray(system32Path));
    }

    wchar_t wow64Path[MAX_PATH];
    if (GetSystemWow64DirectoryW(wow64Path, MAX_PATH)) {
        paths.append(QString::fromWCharArray(wow64Path));
    }

    wchar_t windowsPath[MAX_PATH];
    if (GetWindowsDirectoryW(windowsPath, MAX_PATH)) {
        paths.append(QString::fromWCharArray(windowsPath));
    }

    initialized = true;
    return paths;
}

QStringList cachedFilteredPathDirs(const QString& pathEnv)
{
    static QMutex mutex;
    static QString cachedPathEnv;
    static QStringList cachedDirs;

    QMutexLocker locker(&mutex);
    if (cachedPathEnv == pathEnv) {
        return cachedDirs;
    }

    cachedPathEnv = pathEnv;
    cachedDirs.clear();

    QSet<QString> seen;
    const QStringList rawDirs = pathEnv.split(';', Qt::SkipEmptyParts);
    for (const QString& rawDir : rawDirs) {
        const QString dir = rawDir.trimmed();
        if (dir.isEmpty()) {
            continue;
        }

        // UNC paths can block for a long time when network is unavailable.
        if (dir.startsWith("\\\\")) {
            continue;
        }

        QFileInfo info(dir);
        if (!info.exists() || !info.isDir()) {
            continue;
        }

        const QString normalized = QDir(dir).absolutePath().toLower();
        if (seen.contains(normalized)) {
            continue;
        }

        seen.insert(normalized);
        cachedDirs.append(QDir(dir).absolutePath());
    }

    return cachedDirs;
}
}

PathResolver::ResolveResult PathResolver::resolveDLLPath(const QString& dllName, const QString& applicationDir)
{
    static QMutex cacheMutex;
    static QHash<QString, ResolveResult> resolveCache;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString pathEnv = env.value("PATH");
    const QString cacheKey = QString("%1|%2|%3")
        .arg(dllName.toLower())
        .arg(applicationDir.toLower())
        .arg(QString::number(qHash(pathEnv)));

    {
        QMutexLocker locker(&cacheMutex);
        if (resolveCache.contains(cacheKey)) {
            return resolveCache.value(cacheKey);
        }
    }

    ResolveResult result;
    result.dllName = dllName;
    result.found = false;

    QFileInfo inputFileInfo(dllName);
    if (inputFileInfo.isAbsolute() && inputFileInfo.exists() && inputFileInfo.isFile()) {
        result.foundPath = inputFileInfo.absoluteFilePath();
        result.found = true;
        QMutexLocker locker(&cacheMutex);
        resolveCache.insert(cacheKey, result);
        return result;
    }
    
    // Build search paths according to Windows DLL search order
    QStringList searchPaths;
    QSet<QString> addedSearchPath;
    auto tryAddSearchPath = [&searchPaths, &addedSearchPath](const QString& path) {
        if (path.isEmpty()) {
            return;
        }
        const QString normalized = QDir(path).absolutePath().toLower();
        if (addedSearchPath.contains(normalized)) {
            return;
        }
        addedSearchPath.insert(normalized);
        searchPaths.append(QDir(path).absolutePath());
    };
    
    // 1. Application directory
    if (!applicationDir.isEmpty()) {
        tryAddSearchPath(applicationDir);
    }
    
    // 2/3/4. System paths
    const QStringList systemPaths = cachedSystemPaths();
    for (const QString& path : systemPaths) {
        tryAddSearchPath(path);
    }
    
    // 5. Current directory
    tryAddSearchPath(QDir::currentPath());
    
    // 6. PATH environment variable directories (filtered and cached)
    const QStringList pathDirs = cachedFilteredPathDirs(pathEnv);
    for (const QString& path : pathDirs) {
        tryAddSearchPath(path);
    }
    
    // Search for the DLL in each path
    for (const QString& searchPath : searchPaths) {
        QString fullPath = QDir(searchPath).filePath(dllName);
        result.searchedPaths.append(fullPath);
        
        QFileInfo fileInfo(fullPath);
        if (fileInfo.exists() && fileInfo.isFile()) {
            result.foundPath = fileInfo.absoluteFilePath();
            result.found = true;
            QMutexLocker locker(&cacheMutex);
            resolveCache.insert(cacheKey, result);
            return result;
        }
    }

    QMutexLocker locker(&cacheMutex);
    resolveCache.insert(cacheKey, result);
    return result;
}

QStringList PathResolver::getSystemSearchPaths()
{
    return cachedSystemPaths();
}

bool PathResolver::isSystemDLL(const QString& dllName)
{
    static QMutex cacheMutex;
    static QHash<QString, bool> systemDllCache;

    const QString lowerName = dllName.toLower();
    {
        QMutexLocker locker(&cacheMutex);
        if (systemDllCache.contains(lowerName)) {
            return systemDllCache.value(lowerName);
        }
    }

    // Common Windows system DLLs
    static const QSet<QString> systemDLLs = {
        "kernel32.dll", "user32.dll", "gdi32.dll", "advapi32.dll",
        "shell32.dll", "ole32.dll", "oleaut32.dll", "comctl32.dll",
        "comdlg32.dll", "ws2_32.dll", "msvcrt.dll", "ntdll.dll",
        "rpcrt4.dll", "secur32.dll", "winmm.dll", "version.dll",
        "imagehlp.dll", "dbghelp.dll", "psapi.dll", "iphlpapi.dll",
        "netapi32.dll", "userenv.dll", "winspool.drv", "imm32.dll",
        "msimg32.dll", "setupapi.dll", "wininet.dll", "crypt32.dll",
        "wintrust.dll", "shlwapi.dll", "mpr.dll", "credui.dll"
    };

    bool isSystem = false;
    
    // Check if it's in the known system DLLs list
    if (systemDLLs.contains(lowerName)) {
        isSystem = true;
    }
    
    // Check if it starts with common system prefixes
    if (!isSystem &&
        (lowerName.startsWith("api-ms-win-") ||
         lowerName.startsWith("ext-ms-win-") ||
         lowerName.startsWith("ucrtbase"))) {
        isSystem = true;
    }
    
    // Check if the DLL is in a system directory
    if (!isSystem) {
        QStringList systemPaths = getSystemSearchPaths();
        for (const QString& systemPath : systemPaths) {
            QString fullPath = QDir(systemPath).filePath(dllName);
            if (QFileInfo::exists(fullPath)) {
                isSystem = true;
                break;
            }
        }
    }

    QMutexLocker locker(&cacheMutex);
    systemDllCache.insert(lowerName, isSystem);
    return isSystem;
}
