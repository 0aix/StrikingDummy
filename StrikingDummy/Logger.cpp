#include "stdafx.h"

#include "Logger.h"
#include <fstream>

namespace Logger
{
	std::fstream fs;

	void open()
	{
		fs.open("log.txt", std::fstream::out | std::fstream::app);
	}

	void log(const char* message)
	{
		fs << message;
	}

	void close()
	{
		fs.close();
	}
}