#include "Debugger/DebugWindow.h"

namespace MarCmd
{
	Console::BaseWindowRef createDebugWindow(uint64_t width, uint64_t height)
	{
		auto wndFull = Console::SplitWindow::create(DbgWndName_Full);
		wndFull->resize(width, height);
		wndFull->setRatio(Console::WRT::RelativeLeft, 50);
		{
			auto wndFullLeft = Console::SplitWindow::create(DbgWndName_LeftHalf);
			wndFullLeft->setRatio(Console::WRT::RelativeTop, 50);
			{
				// Disassembler (top)
			}
			{
				auto wndConsoleAndInput = Console::SplitWindow::create(DbgWndName_ConsoleInputSplit);
				wndConsoleAndInput->setRatio(Console::WRT::AbsoluteBottom, 1);
				{
					// Console (top)
					auto wndConsole = Console::SplitWindow::create(DbgWndName_ConsoleSplit);
					wndConsole->setRatio(Console::WRT::AbsoluteTop, 1);
					{
						auto wndConsoleTitle = Console::TextWindow::create(DbgWndName_ConsoleTitle);
						wndConsoleTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
						wndConsoleTitle->addTextFormat(Console::TFC::F_Negative);
						wndConsoleTitle->wrapping(false);
						wndConsole->setTop(wndConsoleTitle);
					}
					{
						auto wndConsoleView = Console::TextWindow::create(DbgWndName_ConsoleView);
						wndConsole->setBottom(wndConsoleView);
					}
					wndConsoleAndInput->setTop(wndConsole);
				}
				{
					// Input (bottom)
					auto wndInput = Console::TextWindow::create(DbgWndName_InputView);
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
			auto wndFullRight = Console::SplitWindow::create(DbgWndName_RightHalfWithSep);
			wndFullRight->setRatio(Console::WRT::AbsoluteLeft, 1);
			{
				// Separator (left)
				auto wndSeparator = Console::TextWindow::create(DbgWndName_Separator);
				wndSeparator->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
				wndSeparator->addTextFormat(Console::TFC::F_Negative);
				wndSeparator->wrapping(false);
				wndFullRight->setLeft(wndSeparator);
			}
			{
				auto wndRight = Console::SplitWindow::create(DbgWndName_RightHalf);
				wndRight->setRatio(Console::WRT::RelativeTop, 65);
				{
					// Memory (top)
					auto wndMemory = Console::SplitWindow::create(DbgWndName_MemorySplit);
					wndMemory->setRatio(Console::WRT::AbsoluteTop, 1);
					{
						auto wndMemoryTitle = Console::TextWindow::create(DbgWndName_MemoryTitle);
						wndMemoryTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
						wndMemoryTitle->addTextFormat(Console::TFC::F_Negative);
						wndMemoryTitle->wrapping(false);
						wndMemory->setTop(wndMemoryTitle);
					}
					{
						auto wndMemoryView = Console::TextWindow::create(DbgWndName_MemoryView);
						wndMemory->setBottom(wndMemoryView);
					}
					wndRight->setTop(wndMemory);
				}
				{
					auto wndModuleCallstack = Console::SplitWindow::create(DbgWndName_ModuleCallstackSplit);
					wndModuleCallstack->setRatio(Console::WRT::RelativeLeft, 50);
					{
						// Module Browser
						auto wndModule = Console::SplitWindow::create(DbgWndName_ModuleSplit);
						wndModule->setRatio(Console::WRT::AbsoluteTop, 1);
						{
							auto wndModuleTitle = Console::TextWindow::create(DbgWndName_ModuleTitle);
							wndModuleTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
							wndModuleTitle->addTextFormat(Console::TFC::F_Negative);
							wndModuleTitle->wrapping(false);
							wndModule->setTop(wndModuleTitle);
						}
						{
							auto wndCallStackView = Console::TextWindow::create(DbgWndName_ModuleView);
							wndModule->setBottom(wndCallStackView);
						}
						wndModuleCallstack->setLeft(wndModule);
					}
					{
						// Callstack (bottom)
						auto wndCallStack = Console::SplitWindow::create(DbgWndName_CallstackSplit);
						wndCallStack->setRatio(Console::WRT::AbsoluteTop, 1);
						{
							auto wndCallStackTitle = Console::TextWindow::create(DbgWndName_CallstackTitle);
							wndCallStackTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
							wndCallStackTitle->addTextFormat(Console::TFC::F_Negative);
							wndCallStackTitle->wrapping(false);
							wndCallStack->setTop(wndCallStackTitle);
						}
						{
							auto wndCallStackView = Console::TextWindow::create(DbgWndName_CallstackView);
							wndCallStack->setBottom(wndCallStackView);
						}
						wndModuleCallstack->setRight(wndCallStack);
					}
					wndRight->setBottom(wndModuleCallstack);
				}
				wndFullRight->setRight(wndRight);
			}
			wndFull->setRight(wndFullRight);
		}

		return std::make_shared<Console::BaseWindow>(wndFull);
	}
}