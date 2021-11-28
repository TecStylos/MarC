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

		auto wndUpper = Console::TextWindow::create();
		auto wndLower = Console::TextWindow::create();

		auto full = Console::SplitWindow::create();
		full->resize(120, 30);
		full->setRatio(Console::WindowRatioType::RelativeLeft, 97);
		full->setTopLeft(wndUpper);
		full->setBottomRight(wndLower);

		wndUpper->addTextFormat(Console::TFC::F_Underline);
		wndUpper->addTextFormat(Console::TFC::FG_Blue);
		wndLower->addTextFormat(Console::TFC::F_Underline);
		wndLower->addTextFormat(Console::TFC::FG_Red);

		full->render(0, 0);

		return 0;
	}
}
