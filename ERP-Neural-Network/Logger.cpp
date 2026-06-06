#include "Logger.h"

#include <cstdio>

#include <mutex>

namespace Logger
{
	static std::mutex PrintMutex;

	void PrintInternal(const char* str)
	{
		const std::lock_guard<std::mutex> lock(PrintMutex);
		printf(str);
	}
};