#include "logger.h"
#include <QCoreApplication>
#include <iostream>

Logger* Logger::s_instance = nullptr;
QMutex Logger::s_mutex;

Logger* Logger::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new Logger();
    }
    return s_instance;
}

Logger::Logger(QObject *parent)
    : QObject(parent)
    , m_logLevel(Info)
    , m_enableFileLogging(true)
    , m_enableConsoleLogging(false)
{
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(logDir);
    m_logFilePath = logDir + "/dll_checker.log";
}

Logger::~Logger()
{
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void Logger::log(LogLevel level, const QString& category, const QString& message)
{
    if (level < m_logLevel) {
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString logLine = QString("[%1] [%2] [%3] %4")
        .arg(timestamp)
        .arg(levelStr)
        .arg(category)
        .arg(message);

    QMutexLocker locker(&m_writeMutex);

    if (m_enableConsoleLogging) {
        if (level >= Error) {
            std::cerr << logLine.toStdString() << std::endl;
        } else {
            std::cout << logLine.toStdString() << std::endl;
        }
    }

    if (m_enableFileLogging) {
        ensureLogFile();
        writeToFile(logLine);
    }

    emit logMessage(timestamp, levelStr, category, message);
}

void Logger::debug(const QString& category, const QString& message)
{
    log(Debug, category, message);
}

void Logger::info(const QString& category, const QString& message)
{
    log(Info, category, message);
}

void Logger::warning(const QString& category, const QString& message)
{
    log(Warning, category, message);
}

void Logger::error(const QString& category, const QString& message)
{
    log(Error, category, message);
}

void Logger::critical(const QString& category, const QString& message)
{
    log(Critical, category, message);
}

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&m_writeMutex);
    m_logLevel = level;
}

Logger::LogLevel Logger::getLogLevel() const
{
    return m_logLevel;
}

void Logger::setEnableFileLogging(bool enable)
{
    QMutexLocker locker(&m_writeMutex);
    if (!enable && m_logFile.isOpen()) {
        m_logFile.close();
    }
    m_enableFileLogging = enable;
}

bool Logger::isFileLoggingEnabled() const
{
    return m_enableFileLogging;
}

void Logger::setEnableConsoleLogging(bool enable)
{
    QMutexLocker locker(&m_writeMutex);
    m_enableConsoleLogging = enable;
}

bool Logger::isConsoleLoggingEnabled() const
{
    return m_enableConsoleLogging;
}

void Logger::setLogFilePath(const QString& path)
{
    QMutexLocker locker(&m_writeMutex);
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
    m_logFilePath = path;
}

QString Logger::getLogFilePath() const
{
    return m_logFilePath;
}

QString Logger::levelToString(LogLevel level) const
{
    switch (level) {
        case Debug:    return "DEBUG";
        case Info:     return "INFO";
        case Warning:  return "WARNING";
        case Error:    return "ERROR";
        case Critical: return "CRITICAL";
        default:       return "UNKNOWN";
    }
}

void Logger::clearLog()
{
    QMutexLocker locker(&m_writeMutex);
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
    QFile::remove(m_logFilePath);
}

void Logger::ensureLogFile()
{
    if (!m_logFile.isOpen()) {
        m_logFile.setFileName(m_logFilePath);
        if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream stream(&m_logFile);
            stream.setCodec("UTF-8");
        }
    }
}

void Logger::writeToFile(const QString& logLine)
{
    if (m_logFile.isOpen()) {
        QTextStream stream(&m_logFile);
        stream.setCodec("UTF-8");
        stream << logLine << "\n";
        stream.flush();
    }
}
