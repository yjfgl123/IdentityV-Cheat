#pragma once
#include <mutex>

class Logger {
private:
	const char* path;
	bool dev;
	std::mutex mtx;
public:
	static Logger* createInstance(const char* path,bool dev);
	void log(const char* format, ...);
};