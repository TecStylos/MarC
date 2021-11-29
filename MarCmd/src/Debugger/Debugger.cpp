#include "Debugger/Debugger.h"

#include <iostream>
#include <thread>

#include "Debugger/ConsoleHelper.h"
#include "Debugger/ConsoleWindow.h"

namespace MarCmd
{
	namespace DbgWndName
	{
		const std::string Full = "Full";
		const std::string LeftHalf = "Left Half";
		const std::string DisasmSplit = "Disassembly Split";
		const std::string DisasmTitle = "Disassembly Title";
		const std::string DisasmView = "Disassembly View";
		const std::string ConsoleInputSplit = "Console & Input";
		const std::string ConsoleSplit = "Console Split";
		const std::string ConsoleTitle = "Console Title";
		const std::string ConsoleView = "Console View";
		const std::string InputView = "Input View";
		const std::string RightHalfWithSep = "Right Half & Separator";
		const std::string Separator = "Separator";
		const std::string RightHalf = "Right Half";
		const std::string MemorySplit = "Memory Split";
		const std::string MemoryTitle = "Memory Title";
		const std::string MemoryView = "Memory View";
		const std::string CallstackSplit = "Callstack Split";
		const std::string CallstackTitle = "Callstack Title";
		const std::string CallstackView = "Callstack View";
	}

	Console::SplitWindowRef createDebugWindow(uint64_t width, uint64_t height)
	{
		auto wndFull = Console::SplitWindow::create(DbgWndName::Full);
		wndFull->resize(width, height);
		wndFull->setRatio(Console::WRT::RelativeLeft, 50);
		{
			auto wndFullLeft = Console::SplitWindow::create(DbgWndName::LeftHalf);
			wndFullLeft->setRatio(Console::WRT::RelativeTop, 50);
			{
				// Disassembler
				auto wndDisasm = Console::SplitWindow::create(DbgWndName::DisasmSplit);
				wndDisasm->setRatio(Console::WRT::AbsoluteTop, 1);
				{
					auto wndDisasmTitle = Console::TextWindow::create(DbgWndName::DisasmTitle);
					wndDisasmTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
					wndDisasmTitle->addTextFormat(Console::TFC::F_Negative);
					wndDisasmTitle->wrapping(false);
					wndDisasm->setTop(wndDisasmTitle);
				}
				{
					auto wndDisasmView = Console::TextWindow::create(DbgWndName::DisasmView);
					wndDisasm->setBottom(wndDisasmView);
				}
				wndFullLeft->setTop(wndDisasm);
			}
			{
				auto wndConsoleAndInput = Console::SplitWindow::create(DbgWndName::ConsoleInputSplit);
				wndConsoleAndInput->setRatio(Console::WRT::AbsoluteBottom, 1);
				{
					// Console
					auto wndConsole = Console::SplitWindow::create(DbgWndName::ConsoleSplit);
					wndConsole->setRatio(Console::WRT::AbsoluteTop, 1);
					{
						auto wndConsoleTitle = Console::TextWindow::create(DbgWndName::ConsoleTitle);
						wndConsoleTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
						wndConsoleTitle->addTextFormat(Console::TFC::F_Negative);
						wndConsoleTitle->wrapping(false);
						wndConsole->setTop(wndConsoleTitle);
					}
					{
						auto wndConsoleView = Console::TextWindow::create(DbgWndName::ConsoleView);
						wndConsole->setBottom(wndConsoleView);
					}
					wndConsoleAndInput->setTop(wndConsole);
				}
				{
					// Input
					auto wndInput = Console::TextWindow::create(DbgWndName::InputView);
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
			auto wndFullRight = Console::SplitWindow::create(DbgWndName::RightHalfWithSep);
			wndFullRight->setRatio(Console::WRT::AbsoluteLeft, 1);
			{
				// Separator
				auto wndSeparator = Console::TextWindow::create(DbgWndName::Separator);
				wndSeparator->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
				wndSeparator->addTextFormat(Console::TFC::F_Negative);
				wndSeparator->wrapping(false);
				wndFullRight->setLeft(wndSeparator);
			}
			{
				auto wndRight = Console::SplitWindow::create(DbgWndName::RightHalf);
				wndRight->setRatio(Console::WRT::RelativeTop, 65);
				{
					// Memory
					auto wndMemory = Console::SplitWindow::create(DbgWndName::MemorySplit);
					wndMemory->setRatio(Console::WRT::AbsoluteTop, 1);
					{
						auto wndMemoryTitle = Console::TextWindow::create(DbgWndName::MemoryTitle);
						wndMemoryTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
						wndMemoryTitle->addTextFormat(Console::TFC::F_Negative);
						wndMemoryTitle->wrapping(false);
						wndMemory->setTop(wndMemoryTitle);
					}
					{
						auto wndMemoryView = Console::TextWindow::create(DbgWndName::MemoryView);
						wndMemory->setBottom(wndMemoryView);
					}
					wndRight->setTop(wndMemory);
				}
				{
					// Callstack
					auto wndCallStack = Console::SplitWindow::create(DbgWndName::CallstackSplit);
					wndCallStack->setRatio(Console::WRT::AbsoluteTop, 1);
					{
						auto wndCallStackTitle = Console::TextWindow::create(DbgWndName::CallstackTitle);
						wndCallStackTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
						wndCallStackTitle->addTextFormat(Console::TFC::F_Negative);
						wndCallStackTitle->wrapping(false);
						wndCallStack->setTop(wndCallStackTitle);
					}
					{
						auto wndCallStackView = Console::TextWindow::create(DbgWndName::CallstackView);
						wndCallStack->setBottom(wndCallStackView);
					}
					wndRight->setBottom(wndCallStack);
				}
				wndFullRight->setRight(wndRight);
			}
			wndFull->setRight(wndFullRight);
		}

		return wndFull;
	}

	int Debugger::run(const Settings& settings)
	{
		auto cd = Console::getDimensions();
		auto wndFull = createDebugWindow(cd.width, cd.height);
		cd = { 0 };

		Console::subTextWndInsert(wndFull, DbgWndName::DisasmTitle, "Disassembly:", 1, 0);
		Console::subTextWndInsert(wndFull, DbgWndName::ConsoleTitle, "Console:", 1, 0);
		Console::subTextWndInsert(wndFull, DbgWndName::InputView, ">> ", 1, 0);
		Console::subTextWndInsert(wndFull, DbgWndName::MemoryTitle, "Memory:", 1, 0);
		Console::subTextWndInsert(wndFull, DbgWndName::CallstackTitle, "Callstack:", 1, 0);

		std::string text = "This is a long text that needs wrapping, I hope it works as I expect it to work. If it should not work, I need to edit my implementation.\n"
			"It also has line breaks.\n"\
			"Like the one before this sentence.\n"
			"It should work as expected.\n"
			"Indentation should also be preserved for line breaks.\n"
			"Here's a tab: '\t' It should occupy two chars in the buffer."
			;
		Console::subTextWndInsert(wndFull, DbgWndName::DisasmView, text, 1, 0);

		bool stopExecution = false;
		while (!stopExecution)
		{
			auto newCD = Console::getDimensions();
			if (newCD.width != cd.width || newCD.height != cd.height)
			{
				cd = newCD;
				wndFull->resize(cd.width, cd.height);
			}

			std::cout << Console::CurVis::Hide;
			wndFull->render(0, 0);
			std::cout << Console::CurVis::Show;
			if (Console::charWaiting())
			{
				switch (Console::getChar())
				{
				case 'n':
				{
					auto wnd = wndFull->getSubWndByName<Console::TextWindow>(DbgWndName::DisasmView);
					wnd->wrapping(!wnd->wrapping());
				}
					break;
				case 'e':
					stopExecution = true;
					break;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		return 0;
	}
}
