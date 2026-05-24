#include "logger.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <iostream>

// Static instance holder
static Logger* g_logger = nullptr;

Logger::Logger()
    : initialized(false)
{
}

Logger::~Logger()
{
    if (logFile.isOpen()) {
        logFile.close();
    }
}

Logger& Logger::instance()
{
    if (!g_logger) {
        g_logger = new Logger();
    }
    return *g_logger;
}

void Logger::init()
{
    instance().initInternal();
}

void Logger::initInternal()
{
    QMutexLocker locker(&logMutex);

    if (initialized) {
        return;
    }

    // Get the executable directory
    QString exePath = QCoreApplication::applicationFilePath();
    QFileInfo fileInfo(exePath);
    QString logDir = fileInfo.absolutePath();
    QString logPath = logDir + "/boombox.log";

    logFile.setFileName(logPath);

    // Open log file in append mode
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        std::cerr << "Failed to open log file: " << logPath.toStdString() << std::endl;
        return;
    }

    initialized = true;

    // Log initialization message (write directly to avoid mutex deadlock)
    QString timestamp = getCurrentTimestamp();
    QString levelStr = levelToString(LogLevel::Info);
    QString startMsg = QString("Logger initialized. Log file: %1").arg(logPath);
    QString logLine = QString("[%1] [%2] [%3] %4\n")
                          .arg(timestamp)
                          .arg(levelStr)
                          .arg("Logger")
                          .arg(startMsg);
    logFile.write(logLine.toUtf8());
    logFile.flush();
    std::cout << logLine.toStdString();
}

void Logger::log(LogLevel level, const QString& component, const QString& message)
{
    instance().logInternal(level, component, message);
}

void Logger::logInternal(LogLevel level, const QString& component, const QString& message)
{
    QMutexLocker locker(&logMutex);

    if (!initialized || !logFile.isOpen()) {
        std::cerr << "Logger not initialized or file not open" << std::endl;
        return;
    }

    // Format: [TIMESTAMP] [LEVEL] [COMPONENT] Message
    QString timestamp = getCurrentTimestamp();
    QString levelStr = levelToString(level);
    QString logLine = QString("[%1] [%2] [%3] %4\n")
                          .arg(timestamp)
                          .arg(levelStr)
                          .arg(component)
                          .arg(message);

    // Write to file
    logFile.write(logLine.toUtf8());
    logFile.flush();

    // Also output to console for debugging
    if (level == LogLevel::Error) {
        std::cerr << logLine.toStdString();
    } else {
        std::cout << logLine.toStdString();
    }
}

void Logger::debug(const QString& component, const QString& message)
{
    log(LogLevel::Debug, component, message);
}

void Logger::info(const QString& component, const QString& message)
{
    log(LogLevel::Info, component, message);
}

void Logger::warn(const QString& component, const QString& message)
{
    log(LogLevel::Warn, component, message);
}

void Logger::error(const QString& component, const QString& message)
{
    log(LogLevel::Error, component, message);
}

QString Logger::levelToString(LogLevel level) const
{
    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

QString Logger::getCurrentTimestamp() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}
