#ifndef PEPARSER_H
#define PEPARSER_H

#include <QString>
#include <QStringList>
#include <QPair>
#include <QDateTime>
#include <Windows.h>
#include <ImageHlp.h>

#pragma comment(lib, "imagehlp.lib")
#pragma comment(lib, "version.lib")

// RAII wrapper for LOADED_IMAGE to ensure proper resource cleanup
class LoadedImageGuard
{
private:
    PLOADED_IMAGE m_image;
    bool m_valid;

public:
    explicit LoadedImageGuard(const QString& filePath) : m_valid(false)
    {
        m_image = new LOADED_IMAGE();
        if (MapAndLoad(filePath.toLocal8Bit().constData(), 
                       NULL, m_image, TRUE, TRUE)) {
            m_valid = true;
        }
    }

    ~LoadedImageGuard()
    {
        if (m_valid && m_image) {
            UnMapAndLoad(m_image);
            delete m_image;
        }
    }

    bool isValid() const { return m_valid; }
    PLOADED_IMAGE get() const { return m_image; }

    // Disable copy
    LoadedImageGuard(const LoadedImageGuard&) = delete;
    LoadedImageGuard& operator=(const LoadedImageGuard&) = delete;
};

class PEParser
{
public:
    enum Architecture {
        Unknown,
        x86,
        x64
    };

    struct PEInfo {
        QString filePath;
        Architecture arch;
        QString fileVersion;
        QString productVersion;
        QStringList dependencies;
        bool isValid;
        QString errorMessage;
        quint64 fileSize;
        QDateTime modifiedTime;
        
        PEInfo() : arch(Unknown), isValid(false), fileSize(0) {}
    };

    // Parse PE file and get all information
    static PEInfo parsePEFile(const QString& filePath);
    
    // Get imported DLLs from PE file
    static QStringList getImportedDLLs(const QString& filePath);
    
    // Get architecture of PE file
    static Architecture getArchitecture(const QString& filePath);
    
    // Get version information
    static QPair<QString, QString> getVersionInfo(const QString& filePath);
    
    // Convert architecture enum to string
    static QString architectureToString(Architecture arch);
};

#endif // PEPARSER_H
