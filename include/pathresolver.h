#ifndef PATHRESOLVER_H
#define PATHRESOLVER_H

#include <QString>
#include <QStringList>

class PathResolver
{
public:
    struct ResolveResult {
        QString dllName;
        QString foundPath;
        bool found;
        QStringList searchedPaths;
        
        ResolveResult() : found(false) {}
    };

    // Resolve DLL path according to Windows DLL search order
    static ResolveResult resolveDLLPath(const QString& dllName, const QString& applicationDir);
    
    // Get system DLL search paths
    static QStringList getSystemSearchPaths();
    
    // Check if a DLL is a system DLL
    static bool isSystemDLL(const QString& dllName);
};

#endif // PATHRESOLVER_H
