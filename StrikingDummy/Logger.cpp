#include "Logger.h"
#include <fstream>
#include <chrono>

namespace Logger
{
	std::fstream fs;
	bool is_open = false;

	void open()
	{
		// not actually time in milliseconds
		long long millis = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
		std::stringstream ss;
		ss << "log-" << millis << ".txt";
		fs.open(ss.str(), std::fstream::out | std::fstream::app);
		is_open = true;
	}

	void log(const char* message)
	{
		if (!is_open)
			open();
		fs << message;
	}

	void close()
	{
		fs.close();
		is_open = false;
	}
}