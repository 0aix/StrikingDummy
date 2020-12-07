#pragma once
#include <sstream>

namespace Logger
{
	void open(std::string suffix);
	void log(const char* message);
	void close();
}