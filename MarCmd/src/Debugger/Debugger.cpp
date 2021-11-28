#include "Debugger/Debugger.h"

#include <iostream>
#include <thread>

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
					wndDisasmTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
					wndDisasmTitle->addTextFormat(Console::TFC::F_Negative);
					wndDisasmTitle->wrapping(false);
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
						wndConsoleTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
						wndConsoleTitle->addTextFormat(Console::TFC::F_Negative);
						wndConsoleTitle->wrapping(false);
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
					wndInput->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
					wndInput->addTextFormat(Console::TFC::F_Negative);
					wndInput->wrapping(false);
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
				wndSeparator->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
				wndSeparator->addTextFormat(Console::TFC::F_Negative);
				wndSeparator->wrapping(false);
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
						wndMemoryTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
						wndMemoryTitle->addTextFormat(Console::TFC::F_Negative);
						wndMemoryTitle->wrapping(false);
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
						wndCallStackTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
						wndCallStackTitle->addTextFormat(Console::TFC::F_Negative);
						wndCallStackTitle->wrapping(false);
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

		Console::Dimensions cd = { 0 };
		wndFull->resize(cd.width, cd.height);

		while (true)
		{
			auto newCD = Console::getDimensions();
			if (newCD.width != cd.width || newCD.height != cd.height)
			{
				cd = newCD;
				wndFull->resize(cd.width, cd.height);

				Console::subTextWndWrite(wndFull, "Disassembly Title", "This is the disassembly title!", 0, 0);
				Console::subTextWndWrite(wndFull, "Console Title", "This is the console title!", 0, 0);

				std::string text = "This is a long text that needs wrapping, I hope it works as I expect it to work. If it should not work, I need to edit my implementation.\n"
					"It also has line breaks.\n"\
					"Like the one before this sentence.\n"
					"It should work as expected.\n"
					"Indentation should also be preserved for line breaks.\n"
					"Here's a tab: '\t' It should occupy two chars in the buffer."
					;
				auto disasmView = wndFull->getSubWindowByName<Console::TextWindow>("Disassembly View");
				if (disasmView)
				{
					disasmView->clearBuffer();
					disasmView->write(text, 0, 0);
				}
			}
			wndFull->render(0, 0);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		return 0;
	}
}
