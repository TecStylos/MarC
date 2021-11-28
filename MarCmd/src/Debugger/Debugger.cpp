#include "Debugger/Debugger.h"

#include <iostream>

#include "Debugger/ConsoleHelper.h"
#include "Debugger/ConsoleWindow.h"

namespace MarCmd
{
	int Debugger::run(const Settings& settings)
	{
		auto wndFull = Console::SplitWindow::create();
		wndFull->setRatio(Console::WRT::RelativeLeft, 50);
		{
			auto wndFullLeft = Console::SplitWindow::create();
			wndFullLeft->setRatio(Console::WRT::RelativeTop, 50);
			{
				// Disassembler
				auto wndDisasm = Console::SplitWindow::create();
				wndDisasm->setRatio(Console::WRT::AbsoluteTop, 1);
				{
					auto wndDisasmTitle = Console::TextWindow::create();
					wndDisasmTitle->addTextFormat(Console::TFC::F_Negative);
					wndDisasm->setTopLeft(wndDisasmTitle);
				}
				{
					auto wndDisasmView = Console::TextWindow::create();
					wndDisasm->setBottomRight(wndDisasmView);
				}
				wndFullLeft->setTopLeft(wndDisasm);
			}
			{
				auto wndConsoleAndInput = Console::SplitWindow::create();
				wndConsoleAndInput->setRatio(Console::WRT::AbsoluteBottom, 1);
				{
					// Console
					auto wndConsole = Console::SplitWindow::create();
					wndConsole->setRatio(Console::WRT::AbsoluteTop, 1);
					{
						auto wndConsoleTitle = Console::TextWindow::create();
						wndConsoleTitle->addTextFormat(Console::TFC::F_Negative);
						wndConsole->setTopLeft(wndConsoleTitle);
					}
					{
						auto wndConsoleView = Console::TextWindow::create();
						wndConsole->setBottomRight(wndConsoleView);
					}
					wndConsoleAndInput->setTopLeft(wndConsole);
				}
				{
					// Input
					auto wndInput = Console::TextWindow::create();
					wndInput->addTextFormat(Console::TFC::F_Negative);
					wndConsoleAndInput->setBottomRight(wndInput);
				}
				wndFullLeft->setBottomRight(wndConsoleAndInput);
			}
			wndFull->setTopLeft(wndFullLeft);
		}
		{
			auto wndSepAndRight = Console::SplitWindow::create();
			wndSepAndRight->setRatio(Console::WRT::AbsoluteLeft, 1);
			{
				// Separator
				auto wndSeparator = Console::TextWindow::create();
				wndSeparator->addTextFormat(Console::TFC::F_Negative);
				wndSepAndRight->setTopLeft(wndSeparator);
			}
			{
				auto wndRight = Console::SplitWindow::create();
				wndRight->setRatio(Console::WRT::RelativeTop, 50);
				{
					// Memory
					auto wndMemory = Console::SplitWindow::create();
					wndMemory->setRatio(Console::WRT::AbsoluteTop, 1);
					{
						auto wndMemoryTitle = Console::TextWindow::create();
						wndMemoryTitle->addTextFormat(Console::TFC::F_Negative);
						wndMemory->setTopLeft(wndMemoryTitle);
					}
					{
						auto wndMemoryView = Console::TextWindow::create();
						wndMemory->setBottomRight(wndMemoryView);
					}
					wndRight->setTopLeft(wndMemory);
				}
				{
					// Callstack
					auto wndCallStack = Console::SplitWindow::create();
					wndCallStack->setRatio(Console::WRT::AbsoluteTop, 1);
					{
						auto wndCallStackTitle = Console::TextWindow::create();
						wndCallStackTitle->addTextFormat(Console::TFC::F_Negative);
						wndCallStack->setTopLeft(wndCallStackTitle);
					}
					{
						auto wndCallStackView = Console::TextWindow::create();
						wndCallStack->setBottomRight(wndCallStackView);
					}
					wndRight->setBottomRight(wndCallStack);
				}
				wndSepAndRight->setBottomRight(wndRight);
			}
			wndFull->setBottomRight(wndSepAndRight);
		}

		auto cd = Console::getDimensions();
		wndFull->resize(cd.width, cd.height);
		wndFull->render(0, 0);

		return 0;
	}
}
