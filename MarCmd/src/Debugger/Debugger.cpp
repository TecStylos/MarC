#include "Debugger/Debugger.h"

#include <iostream>

namespace MarCmd
{
	int Debugger::run(const Settings& settings)
	{
		std::cout << getChar();
		return 0;
	}
}
