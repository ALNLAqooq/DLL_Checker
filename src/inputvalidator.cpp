#include "inputvalidator.h"
#include "logger.h"
#include <QRegularExpression>
#include <QStorageInfo>

const QStringList InputValidator::s_validExtensions = QStringList() << ".exe" << ".dll";
const int InputValidator::s_maxPathLength = 260;

InputValidator::InputValidator(QObject *parent)
    : QObject(parent)
{
}

InputValidator::~InputValidator()
{
}

InputValidator::ValidationResult InputValidator::validateFilePath(const QString& filePath, const QStringList& allowedExtensions)
{
    LOG_DEBUG("InputValidator", QString("验证文件路径: %1").arg(filePath));
    
    // Check for empty input
    if (filePath.trimmed().isEmpty()) {
        LOG_WARNING("InputValidator", "文件路径为空");
        return EmptyInput;
    }
    
    // Check for path too long
    if (filePath.length() > s_maxPathLength) {
        LOG_WARNING("InputValidator", QString("文件路径过长: %1 字符").arg(filePath.length()));
        return PathTooLong;
    }
    
    // Check for invalid characters
    if (containsInvalidCharacters(filePath)) {
        LOG_WARNING("InputValidator", "文件路径包含非法字符");
        return ContainsInvalidChars;
    }
    
    // Check if path exists
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        LOG_WARNING("InputValidator", QString("文件不存在: %1").arg(filePath));
        return PathDoesNotExist;
    }
    
    // Check if it's a file (not a directory)
    if (!fileInfo.isFile()) {
        LOG_WARNING("InputValidator", QString("路径不是文件: %1").arg(filePath));
        return InvalidPath;
    }
    
    // Check read permission
    if (!hasReadPermission(filePath)) {
        LOG_WARNING("InputValidator", QString("无读取权限: %1").arg(filePath));
        return PathNotReadable;
    }
    
    // Check file extension
    if (!allowedExtensions.isEmpty()) {
        QString extension = fileInfo.suffix().toLower();
        bool validExtension = false;
        for (const QString& ext : allowedExtensions) {
            if (ext.startsWith('.')) {
                if (extension == ext.mid(1)) {
                    validExtension = true;
                    break;
                }
            } else {
                if (extension == ext.toLower()) {
                    validExtension = true;
                    break;
                }
            }
        }
        if (!validExtension) {
            LOG_WARNING("InputValidator", QString("无效的文件扩展名: %1").arg(extension));
            return InvalidExtension;
        }
    }
    
    LOG_DEBUG("InputValidator", "文件路径验证通过");
    return Valid;
}

InputValidator::ValidationResult InputValidator::validateDirectoryPath(const QString& dirPath, bool checkWritePermission)
{
    LOG_DEBUG("InputValidator", QString("验证目录路径: %1 (检查写入权限: %2)").arg(dirPath).arg(checkWritePermission));
    
    // Check for empty input
    if (dirPath.trimmed().isEmpty()) {
        LOG_WARNING("InputValidator", "目录路径为空");
        return EmptyInput;
    }
    
    // Check for path too long
    if (dirPath.length() > s_maxPathLength) {
        LOG_WARNING("InputValidator", QString("目录路径过长: %1 字符").arg(dirPath.length()));
        return PathTooLong;
    }
    
    // Check for invalid characters
    if (containsInvalidCharacters(dirPath)) {
        LOG_WARNING("InputValidator", "目录路径包含非法字符");
        return ContainsInvalidChars;
    }
    
    // Check if path exists
    QDir dir(dirPath);
    if (!dir.exists()) {
        LOG_WARNING("InputValidator", QString("目录不存在: %1").arg(dirPath));
        return PathDoesNotExist;
    }
    
    // Check if it's a directory (not a file)
    QFileInfo dirInfo(dirPath);
    if (!dirInfo.isDir()) {
        LOG_WARNING("InputValidator", QString("路径不是目录: %1").arg(dirPath));
        return InvalidPath;
    }
    
    // Check read permission
    if (!hasReadPermission(dirPath)) {
        LOG_WARNING("InputValidator", QString("无读取权限: %1").arg(dirPath));
        return PathNotReadable;
    }
    
    // Check write permission if requested
    if (checkWritePermission && !hasWritePermission(dirPath)) {
        LOG_WARNING("InputValidator", QString("无写入权限: %1").arg(dirPath));
        return PathNotWritable;
    }
    
    LOG_DEBUG("InputValidator", "目录路径验证通过");
    return Valid;
}

InputValidator::ValidationResult InputValidator::validateTargetDirectory(const QString& dirPath)
{
    LOG_DEBUG("InputValidator", QString("验证目标目录: %1").arg(dirPath));
    
    // Check for empty input
    if (dirPath.trimmed().isEmpty()) {
        LOG_WARNING("InputValidator", "目标目录路径为空");
        return EmptyInput;
    }
    
    // Check for path too long
    if (dirPath.length() > s_maxPathLength) {
        LOG_WARNING("InputValidator", QString("目标目录路径过长: %1 字符").arg(dirPath.length()));
        return PathTooLong;
    }
    
    // Check for invalid characters
    if (containsInvalidCharacters(dirPath)) {
        LOG_WARNING("InputValidator", "目标目录路径包含非法字符");
        return ContainsInvalidChars;
    }
    
    // Check if directory exists
    QDir dir(dirPath);
    if (!dir.exists()) {
        // Try to create the directory
        if (!dir.mkpath(".")) {
            LOG_ERROR("InputValidator", QString("无法创建目标目录: %1").arg(dirPath));
            return DirectoryAccessDenied;
        }
        LOG_INFO("InputValidator", QString("成功创建目标目录: %1").arg(dirPath));
    }
    
    // Check write permission
    if (!hasWritePermission(dirPath)) {
        LOG_WARNING("InputValidator", QString("目标目录无写入权限: %1").arg(dirPath));
        return PathNotWritable;
    }
    
    LOG_DEBUG("InputValidator", "目标目录验证通过");
    return Valid;
}

QString InputValidator::getErrorMessage(ValidationResult result)
{
    switch (result) {
        case Valid:
            return tr("验证通过");
        case InvalidPath:
            return tr("路径无效");
        case PathDoesNotExist:
            return tr("路径不存在");
        case PathNotReadable:
            return tr("无读取权限");
        case PathNotWritable:
            return tr("无写入权限");
        case InvalidExtension:
            return tr("文件扩展名无效");
        case EmptyInput:
            return tr("输入为空");
        case PathTooLong:
            return tr("路径过长");
        case ContainsInvalidChars:
            return tr("路径包含非法字符");
        case FileIsLocked:
            return tr("文件被锁定");
        case DirectoryAccessDenied:
            return tr("目录访问被拒绝");
        case SystemDirectory:
            return tr("系统目录");
        case PermissionDenied:
            return tr("权限不足");
        default:
            return tr("未知错误");
    }
}

bool InputValidator::hasReadPermission(const QString& path)
{
    QFileInfo fileInfo(path);
    return fileInfo.isReadable();
}

bool InputValidator::hasWritePermission(const QString& path)
{
    QFileInfo fileInfo(path);
    return fileInfo.isWritable();
}

bool InputValidator::isFileLocked(const QString& filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        return false;
    }
    
    // Try to open the file in read-only mode
    if (file.open(QIODevice::ReadOnly)) {
        file.close();
        return false;
    }
    
    // If we can't open it, it might be locked
    return true;
}

bool InputValidator::isSystemDirectory(const QString& dirPath)
{
    QString normalizedPath = QDir::cleanPath(dirPath).toLower();
    
    QStringList systemDirs = QStringList()
        << "c:\\windows"
        << "c:\\windows\\system32"
        << "c:\\windows\\syswow64"
        << "c:\\program files"
        << "c:\\program files (x86)"
        << "c:\\programdata";
    
    for (const QString& sysDir : systemDirs) {
        if (normalizedPath.startsWith(sysDir)) {
            return true;
        }
    }
    
    return false;
}

bool InputValidator::containsInvalidCharacters(const QString& path)
{
    // Windows invalid characters: < > : " / \ | ? *
    // Note: Colon is only allowed in drive letters (e.g., "C:")
    // Remove drive letter prefix for validation
    QString testPath = path;
    if (testPath.length() >= 2 && testPath[1] == ':' && 
        (testPath[0].isLetter() || testPath[0].toLower() >= 'a' && testPath[0].toLower() <= 'z')) {
        testPath = testPath.mid(2);
    }
    
    QRegularExpression invalidChars(R"([<>:"|?*])");
    return invalidChars.match(testPath).hasMatch();
}

QStringList InputValidator::getValidFileExtensions()
{
    return s_validExtensions;
}

QString InputValidator::sanitizePath(const QString& path)
{
    QString sanitized = path.trimmed();
    
    // Remove any trailing separators
    while (sanitized.endsWith('/') || sanitized.endsWith('\\')) {
        sanitized = sanitized.left(sanitized.length() - 1);
    }
    
    // Replace multiple consecutive separators with single separator
    sanitized.replace(QRegularExpression("/+"), "/");
    sanitized.replace(QRegularExpression("\\\\+"), "\\");
    
    return sanitized;
}
