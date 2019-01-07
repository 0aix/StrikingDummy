#include "Logger.h"
#include <fstream>

namespace Logger
{
	std::fstream fs;
	bool is_open = false;

	void open()
	{
		fs.open("log.txt", std::fstream::out | std::fstream::app);
		is_open = true;
	}

	void log(const char* message)
	{
		if (!is_open)
			open();
		fs << message << std::endl;
	}

	void close()
	{
		fs.close();
		is_open = false;
	}
}