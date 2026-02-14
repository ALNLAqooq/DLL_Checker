#ifndef INPUTVALIDATOR_H
#define INPUTVALIDATOR_H

#include <QObject>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QFileDevice>

class InputValidator : public QObject
{
    Q_OBJECT

public:
    enum ValidationResult {
        Valid,
        InvalidPath,
        PathDoesNotExist,
        PathNotReadable,
        PathNotWritable,
        InvalidExtension,
        EmptyInput,
        PathTooLong,
        ContainsInvalidChars,
        FileIsLocked,
        DirectoryAccessDenied,
        SystemDirectory,
        PermissionDenied
    };

    static ValidationResult validateFilePath(const QString& filePath, const QStringList& allowedExtensions = QStringList());
    static ValidationResult validateDirectoryPath(const QString& dirPath, bool checkWritePermission = false);
    static ValidationResult validateTargetDirectory(const QString& dirPath);
    
    static QString getErrorMessage(ValidationResult result);
    
    static bool hasReadPermission(const QString& path);
    static bool hasWritePermission(const QString& path);
    static bool isFileLocked(const QString& filePath);
    static bool isSystemDirectory(const QString& dirPath);
    static bool containsInvalidCharacters(const QString& path);
    
    static QStringList getValidFileExtensions();
    static QString sanitizePath(const QString& path);

private:
    explicit InputValidator(QObject *parent = nullptr);
    ~InputValidator();
    
    static const QStringList s_validExtensions;
    static const int s_maxPathLength;
};

#endif // INPUTVALIDATOR_H
