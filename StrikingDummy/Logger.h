#pragma once
#include <sstream>

namespace Logger
{
	void open();
	void log(const char* message);
	void close();
}