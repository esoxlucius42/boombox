#include <QApplication>
#include <QFile>
#include "logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Initialize logger after creating QApplication
    Logger::init();
    
    // Test logging
    Logger::info("Main", "Application startup");
    Logger::debug("Main", "This is a debug message");
    Logger::warn("Main", "This is a warning message");
    Logger::error("Main", "This is an error message");
    Logger::info("Main", "Logging test complete");

    return 0;
}
