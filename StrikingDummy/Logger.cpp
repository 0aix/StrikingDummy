#include "Logger.h"
#include <fstream>
#include <chrono>

namespace Logger
{
	std::fstream fs;
	bool is_open = false;

	void open()
	{
		long long seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		std::stringstream ss;
		ss << "log-" << seconds << ".txt";
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