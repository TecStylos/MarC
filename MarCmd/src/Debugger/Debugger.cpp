#include "Debugger/Debugger.h"

#include <iostream>

#include "Debugger/ConsoleHelper.h"
#include "Debugger/ConsoleWindow.h"

namespace MarCmd
{
	int Debugger::run(const Settings& settings)
	{
		auto wndFull = Console::SplitWindow::create("Full");
		wndFull->setRatio(Console::WRT::RelativeLeft, 50);
		{
			auto wndFullLeft = Console::SplitWindow::create("Full Left");
			wndFullLeft->setRatio(Console::WRT::RelativeTop, 50);
			{
				// Disassembler
				auto wndDisasm = Console::SplitWindow::create("Disassembly");
				wndDisasm->setRatio(Console::WRT::AbsoluteTop, 1);
				{
					auto wndDisasmTitle = Console::TextWindow::create("Disassembly Title");
					wndDisasmTitle->addTextFormat(Console::TFC::F_Negative);
					wndDisasm->setTop(wndDisasmTitle);
				}
				{
					auto wndDisasmView = Console::TextWindow::create("Disassembly View");
					wndDisasm->setBottom(wndDisasmView);
				}
				wndFullLeft->setTop(wndDisasm);
			}
			{
				auto wndConsoleAndInput = Console::SplitWindow::create("Console & Input");
				wndConsoleAndInput->setRatio(Console::WRT::AbsoluteBottom, 1);
				{
					// Console
					auto wndConsole = Console::SplitWindow::create("Console");
					wndConsole->setRatio(Console::WRT::AbsoluteTop, 1);
					{
						auto wndConsoleTitle = Console::TextWindow::create("Console Title");
						wndConsoleTitle->addTextFormat(Console::TFC::F_Negative);
						wndConsole->setTop(wndConsoleTitle);
					}
					{
						auto wndConsoleView = Console::TextWindow::create("Console View");
						wndConsole->setBottom(wndConsoleView);
					}
					wndConsoleAndInput->setTop(wndConsole);
				}
				{
					// Input
					auto wndInput = Console::TextWindow::create("Input");
					wndInput->addTextFormat(Console::TFC::F_Negative);
					wndConsoleAndInput->setBottom(wndInput);
				}
				wndFullLeft->setBottom(wndConsoleAndInput);
			}
			wndFull->setLeft(wndFullLeft);
		}
		{
			auto wndFullRight = Console::SplitWindow::create("Full Right");
			wndFullRight->setRatio(Console::WRT::AbsoluteLeft, 1);
			{
				// Separator
				auto wndSeparator = Console::TextWindow::create("Separator");
				wndSeparator->addTextFormat(Console::TFC::F_Negative);
				wndFullRight->setLeft(wndSeparator);
			}
			{
				auto wndRight = Console::SplitWindow::create("Right");
				wndRight->setRatio(Console::WRT::RelativeTop, 65);
				{
					// Memory
					auto wndMemory = Console::SplitWindow::create("Memory");
					wndMemory->setRatio(Console::WRT::AbsoluteTop, 1);
					{
						auto wndMemoryTitle = Console::TextWindow::create("Memory Title");
						wndMemoryTitle->addTextFormat(Console::TFC::F_Negative);
						wndMemory->setTop(wndMemoryTitle);
					}
					{
						auto wndMemoryView = Console::TextWindow::create("Memory Vie");
						wndMemory->setBottom(wndMemoryView);
					}
					wndRight->setTop(wndMemory);
				}
				{
					// Callstack
					auto wndCallStack = Console::SplitWindow::create("Callstack");
					wndCallStack->setRatio(Console::WRT::AbsoluteTop, 1);
					{
						auto wndCallStackTitle = Console::TextWindow::create("Callstack Title");
						wndCallStackTitle->addTextFormat(Console::TFC::F_Negative);
						wndCallStack->setTop(wndCallStackTitle);
					}
					{
						auto wndCallStackView = Console::TextWindow::create("Callstack View");
						wndCallStack->setBottom(wndCallStackView);
					}
					wndRight->setBottom(wndCallStack);
				}
				wndFullRight->setRight(wndRight);
			}
			wndFull->setRight(wndFullRight);
		}

		auto cd = Console::getDimensions();
		wndFull->resize(cd.width, cd.height);

		if (!Console::subTextWndWrite(wndFull, "Disassembly Title", "This is the disassembly title!", 0, 0))
			throw std::runtime_error("Unable to find the 'Disassembly Title' window!");

		if (!Console::subTextWndWrite(wndFull, "Console Title", "This is the console title!", 0, 0))
			throw std::runtime_error("Unable to find the 'Console Title' window!");

		wndFull->render(0, 0);

		return 0;
	}
}
