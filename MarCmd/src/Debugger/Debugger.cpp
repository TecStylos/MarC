#include "Debugger/Debugger.h"

#include <iostream>

#include "Debugger/ConsoleHelper.h"

namespace MarCmd
{
	int Debugger::run(const Settings& settings)
	{
		auto cd = getConsoleDimensions();
		std::cout << "Width:  " << cd.width << std::endl;
		std::cout << "Height: " << cd.height << std::endl;
		std::cout << "Press any key to exit";
		getChar();
		return 0;
	}
}
