#include "peparser.h"
#include <QFile>
#include <QFileInfo>
#include <QDateTime>

PEParser::PEInfo PEParser::parsePEFile(const QString& filePath)
{
    PEInfo info;
    info.filePath = filePath;
    info.isValid = false;
    
    // Check if file exists
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        info.errorMessage = QString("文件不存在: %1\n\n"
            "请检查文件路径是否正确，或文件是否已被移动或删除。").arg(filePath);
        return info;
    }
    
    info.fileSize = fileInfo.size();
    info.modifiedTime = fileInfo.lastModified();
    
    // Get architecture
    info.arch = getArchitecture(filePath);
    if (info.arch == Unknown) {
        info.errorMessage = QString("无效的PE文件或不支持的架构: %1\n\n"
            "可能的原因：\n"
            "1. 文件不是有效的Windows可执行文件（.exe或.dll）\n"
            "2. 文件可能已损坏\n"
            "3. 文件架构不受支持（仅支持x86和x64）").arg(filePath);
        return info;
    }
    
    // Get imported DLLs
    info.dependencies = getImportedDLLs(filePath);
    
    // Get version information
    QPair<QString, QString> versions = getVersionInfo(filePath);
    info.fileVersion = versions.first;
    info.productVersion = versions.second;
    
    info.isValid = true;
    return info;
}

QStringList PEParser::getImportedDLLs(const QString& filePath)
{
    QStringList dlls;
    
    LoadedImageGuard imageGuard(filePath);
    if (!imageGuard.isValid()) {
        return dlls;
    }
    
    PLOADED_IMAGE loadedImage = imageGuard.get();
    
    // Get the import descriptor
    ULONG size;
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(
        loadedImage->MappedAddress, FALSE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size);
    
    if (importDesc) {
        // Iterate through import descriptors
        while (importDesc->Name != 0) {
            // Get DLL name
            PSTR dllName = (PSTR)ImageRvaToVa(loadedImage->FileHeader, 
                                             loadedImage->MappedAddress,
                                             importDesc->Name, NULL);
            if (dllName) {
                dlls.append(QString::fromLatin1(dllName));
            }
            importDesc++;
        }
    }
    
    return dlls;
}

PEParser::Architecture PEParser::getArchitecture(const QString& filePath)
{
    // Open file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return Unknown;
    }
    
    // Read DOS header
    IMAGE_DOS_HEADER dosHeader;
    if (file.read((char*)&dosHeader, sizeof(dosHeader)) != sizeof(dosHeader)) {
        file.close();
        return Unknown;
    }
    
    // Check DOS signature
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        file.close();
        return Unknown;
    }
    
    // Seek to PE header
    file.seek(dosHeader.e_lfanew);
    
    // Read PE signature
    DWORD peSignature;
    if (file.read((char*)&peSignature, sizeof(peSignature)) != sizeof(peSignature)) {
        file.close();
        return Unknown;
    }
    
    // Check PE signature
    if (peSignature != IMAGE_NT_SIGNATURE) {
        file.close();
        return Unknown;
    }
    
    // Read file header
    IMAGE_FILE_HEADER fileHeader;
    if (file.read((char*)&fileHeader, sizeof(fileHeader)) != sizeof(fileHeader)) {
        file.close();
        return Unknown;
    }
    
    file.close();
    
    // Determine architecture from Machine field
    switch (fileHeader.Machine) {
        case IMAGE_FILE_MACHINE_I386:
            return x86;
        case IMAGE_FILE_MACHINE_AMD64:
            return x64;
        default:
            return Unknown;
    }
}

QPair<QString, QString> PEParser::getVersionInfo(const QString& filePath)
{
    QPair<QString, QString> versions("", "");
    
    // Convert to wide string
    std::wstring wFilePath = filePath.toStdWString();
    
    // Get version info size
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(wFilePath.c_str(), &handle);
    if (size == 0) {
        return versions;
    }
    
    // Allocate buffer using QByteArray for automatic memory management
    QByteArray buffer(size, 0);
    if (!GetFileVersionInfoW(wFilePath.c_str(), handle, size, buffer.data())) {
        return versions;
    }
    
    // Get fixed file info
    VS_FIXEDFILEINFO* fileInfo = nullptr;
    UINT len = 0;
    if (VerQueryValueW(buffer.data(), L"\\", (LPVOID*)&fileInfo, &len)) {
        // Extract file version
        DWORD fileVersionMS = fileInfo->dwFileVersionMS;
        DWORD fileVersionLS = fileInfo->dwFileVersionLS;
        versions.first = QString("%1.%2.%3.%4")
            .arg(HIWORD(fileVersionMS))
            .arg(LOWORD(fileVersionMS))
            .arg(HIWORD(fileVersionLS))
            .arg(LOWORD(fileVersionLS));
        
        // Extract product version
        DWORD productVersionMS = fileInfo->dwProductVersionMS;
        DWORD productVersionLS = fileInfo->dwProductVersionLS;
        versions.second = QString("%1.%2.%3.%4")
            .arg(HIWORD(productVersionMS))
            .arg(LOWORD(productVersionMS))
            .arg(HIWORD(productVersionLS))
            .arg(LOWORD(productVersionLS));
    }
    
    return versions;
}

QString PEParser::architectureToString(Architecture arch)
{
    switch (arch) {
        case x86: return "x86";
        case x64: return "x64";
        default: return "Unknown";
    }
}
