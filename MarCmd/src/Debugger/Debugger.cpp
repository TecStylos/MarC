#include "Debugger/Debugger.h"

#include <iostream>

#include "Debugger/ConsoleHelper.h"
#include "Debugger/ConsoleWindow.h"

namespace MarCmd
{
	int Debugger::run(const Settings& settings)
	{
		auto cd = Console::getDimensions();
		std::cout << "Width:  " << cd.width << std::endl;
		std::cout << "Height: " << cd.height << std::endl;

		for (int i = 0; i < 20; ++i)
			std::cout << std::endl;

		Console::Window wnd(20, 10);
		wnd.write("#", 0, 0);
		wnd.write("Hello world this is a long text that does not fit", 0, 1);
		wnd.write("#", 0, 9);
		wnd.write("#", 19, 0);
		wnd.write("#", 19, 9);
		wnd.addTextFormat(Console::TextFormat::FG_Black);
		wnd.addTextFormat(Console::TextFormat::BG_White);
		wnd.render(5, 4);
		return 0;
	}
}
