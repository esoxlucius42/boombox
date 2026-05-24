#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QMutex>
#include <memory>

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

class Logger {
public:
    // Initialize the logger (call from main before QApplication)
    static void init();

    // Log with explicit level
    static void log(LogLevel level, const QString& component, const QString& message);

    // Convenience methods
    static void debug(const QString& component, const QString& message);
    static void info(const QString& component, const QString& message);
    static void warn(const QString& component, const QString& message);
    static void error(const QString& component, const QString& message);

private:
    Logger();
    ~Logger();

    static Logger& instance();

    void initInternal();
    void logInternal(LogLevel level, const QString& component, const QString& message);
    QString levelToString(LogLevel level) const;
    QString getCurrentTimestamp() const;

    QFile logFile;
    QMutex logMutex;
    bool initialized = false;

    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

#endif // LOGGER_H
