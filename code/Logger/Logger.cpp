#include "Logger.h"
#include <stdio.h>
#include <fstream>
#include <windows.h>

Logger* Logger::createInstance(const char* path,bool dev)
{
	remove(path);
	Logger* logger =  new Logger;
	logger->path = path;
    logger->dev = dev;
	return logger;
}

void Logger::log(const char* format, ...)
{
    mtx.lock();
    if (this->dev) {
        std::time_t now = std::time(nullptr);
        char timeStr[20];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        std::ofstream logFile(this->path, std::ios::app);
        if (logFile.is_open()) {
            logFile << "[" << timeStr << "] " << buffer << std::endl;
            logFile.close();
        }
    }
    mtx.unlock();
}
