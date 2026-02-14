#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <QMutex>
#include <QStandardPaths>
#include <QDir>

class Logger : public QObject
{
    Q_OBJECT

public:
    enum LogLevel {
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    static Logger* instance();

    void log(LogLevel level, const QString& category, const QString& message);
    void debug(const QString& category, const QString& message);
    void info(const QString& category, const QString& message);
    void warning(const QString& category, const QString& message);
    void error(const QString& category, const QString& message);
    void critical(const QString& category, const QString& message);

    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;

    void setEnableFileLogging(bool enable);
    bool isFileLoggingEnabled() const;

    void setEnableConsoleLogging(bool enable);
    bool isConsoleLoggingEnabled() const;

    void setLogFilePath(const QString& path);
    QString getLogFilePath() const;

    QString levelToString(LogLevel level) const;

    void clearLog();

signals:
    void logMessage(const QString& timestamp, const QString& level, 
                    const QString& category, const QString& message);

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    void ensureLogFile();
    void writeToFile(const QString& logLine);

    static Logger* s_instance;
    static QMutex s_mutex;

    LogLevel m_logLevel;
    bool m_enableFileLogging;
    bool m_enableConsoleLogging;
    QString m_logFilePath;
    QFile m_logFile;
    QMutex m_writeMutex;
};

#define LOG_DEBUG(category, message) Logger::instance()->debug(category, message)
#define LOG_INFO(category, message) Logger::instance()->info(category, message)
#define LOG_WARNING(category, message) Logger::instance()->warning(category, message)
#define LOG_ERROR(category, message) Logger::instance()->error(category, message)
#define LOG_CRITICAL(category, message) Logger::instance()->critical(category, message)

#endif // LOGGER_H
