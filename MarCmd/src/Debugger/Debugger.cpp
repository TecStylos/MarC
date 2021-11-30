#include "Debugger/Debugger.h"

#include <iostream>
#include <thread>

#include "MarCmdExeInfoLoader.h"

namespace MarCmd
{
	DisasmWindow::DisasmWindow(const std::string& name, MarC::InterpreterRef interpreter, uint64_t modIndex)
		: Console::SplitWindow(name)
	{
		m_interpreter = interpreter;
		m_modIndex = modIndex;

		auto& mod = interpreter->getExeInfo()->modules[modIndex];
		auto& mem = mod->codeMemory;

		uint64_t nDisassembled = 0;
		while (nDisassembled < mem->size())
		{
			ModDisasmInfo::InsInfo insInfo;
			insInfo.data = MarC::Disassembler::disassemble((char*)mem->getBaseAddress() + nDisassembled);
			insInfo.str = MarC::DisAsmInsInfoToString(insInfo.data, m_interpreter->getExeInfo()->symbols);
			m_modDisasmInfo.ins.push_back(insInfo);
			m_modDisasmInfo.instructionOffsets.push_back(nDisassembled);
			nDisassembled += m_modDisasmInfo.ins.back().data.rawData.size();
		}

		this->setRatio(Console::WRT::AbsoluteTop, 1);
		{
			auto wndDisasmTitle = Console::TextWindow::create("Title");
			wndDisasmTitle->addTextFormat(Console::TextFormat::ColorFG(150, 150, 150));
			wndDisasmTitle->addTextFormat(Console::TFC::F_Negative);
			wndDisasmTitle->wrapping(false);
			this->setTop(wndDisasmTitle);
		}
		{
			auto wndDisasmViewSplit = Console::SplitWindow::create("View");
			wndDisasmViewSplit->setRatio(Console::WRT::AbsoluteLeft, 3);
			{
				auto wndDisasmViewControlSplit = Console::SplitWindow::create("Control");
				wndDisasmViewControlSplit->setRatio(Console::WRT::AbsoluteLeft, 2);
				{
					auto wndDisasmViewControlInsPtr = Console::TextWindow::create("Line Marker");
					wndDisasmViewControlInsPtr->insert("->", 0, 0);

					wndDisasmViewControlSplit->setLeft(wndDisasmViewControlInsPtr);
				}
				{
					auto wndDisasmViewControlBreakpoints = Console::TextWindow::create("Breakpoints");
					wndDisasmViewControlSplit->setRight(wndDisasmViewControlBreakpoints);
				}
				wndDisasmViewSplit->setLeft(wndDisasmViewControlSplit);
			}
			{
				auto wndDisasmViewCode = Console::TextWindow::create("Code");
				wndDisasmViewCode->wrapping(false);
				wndDisasmViewSplit->setRight(wndDisasmViewCode);
			}
			this->setBottom(wndDisasmViewSplit);
		}

		auto wndDisasm = getSubWnd<Console::TextWindow>("Code");
		for (auto& ins : m_modDisasmInfo.ins)
			wndDisasm->append(ins.str + "\n");
	}

	void DisasmWindow::handleKeyPress(char key)
	{
		switch (key)
		{
		case 'r':
			break;
		default:
			;
		}

		Window::handleKeyPress(key);
	}

	void DisasmWindow::refresh()
	{
		MarC::BC_MemAddress exeAddr = m_interpreter->getRegister(MarC::BC_MEM_REG_CODE_POINTER).as_ADDR;
		uint64_t modIndex = exeAddr.asCode.page;
		uint64_t offset = exeAddr.asCode.addr;
		int64_t line = MarC::searchBinary(offset, m_modDisasmInfo.instructionOffsets);
		{
			auto wndDisasmTitle = this->getSubWnd<Console::TextWindow>("Title");
			auto wndDisasmViewControlInsPtr = this->getSubWnd<Console::TextWindow>("Line Marker");
			auto wndDisasmViewControlBreakpoints = this->getSubWnd<Console::TextWindow>("Breakpoints");
			auto wndDisasmViewCode = this->getSubWnd<Console::TextWindow>("Code");
			wndDisasmTitle->replace("Disassembly: " + m_interpreter->getExeInfo()->modules[modIndex]->moduleName, 1, 0);
			int64_t mid = wndDisasmViewCode->getHeight() / 2;
			wndDisasmViewControlInsPtr->setScroll(-mid);
			wndDisasmViewControlBreakpoints->setScroll(-mid);
			wndDisasmViewCode->setScroll(-mid + line);
		}
	}

	DisasmWindowRef DisasmWindow::create(const std::string& name, MarC::InterpreterRef interpreter, uint64_t modIndex)
	{
		return std::shared_ptr<DisasmWindow>(new DisasmWindow(name, interpreter, modIndex));
	}

	int Debugger::run(const Settings& settings)
	{
		auto dbgr = Debugger(settings);
		return dbgr.run();
	}

	Debugger::Debugger(const Settings& settings)
		: m_settings(settings)
	{
		m_exeInfo = loadExeInfo(m_settings);

		for (auto& sym : m_exeInfo->symbols)
		{
			if (sym.usage == MarC::SymbolUsage::Address &&
				sym.value.as_ADDR.base != MarC::BC_MEM_BASE_CODE_MEMORY &&
				sym.value.as_ADDR.base != MarC::BC_MEM_BASE_REGISTER
				)
				m_maxPrintSymLen = std::max(m_maxPrintSymLen, sym.name.size());
		}

		m_interpreter = MarC::Interpreter::create(m_exeInfo);

		m_vecWndDisasm.resize(m_exeInfo->modules.size());
		for (uint64_t i = 0; i < m_exeInfo->modules.size(); ++i)
			m_vecWndDisasm[i] = DisasmWindow::create(DbgWndName_Disasm, m_interpreter, i);
		m_wndDisasm = m_vecWndDisasm[0];

		m_exeThreadData.regDatatypes[MarC::BC_MEM_REG_CODE_POINTER] = MarC::BC_DT_ADDR;
		m_exeThreadData.regDatatypes[MarC::BC_MEM_REG_STACK_POINTER] = MarC::BC_DT_ADDR;
		m_exeThreadData.regDatatypes[MarC::BC_MEM_REG_FRAME_POINTER] = MarC::BC_DT_ADDR;
		m_exeThreadData.regDatatypes[MarC::BC_MEM_REG_LOOP_COUNTER] = MarC::BC_DT_UNKNOWN;
		m_exeThreadData.regDatatypes[MarC::BC_MEM_REG_ACCUMULATOR] = MarC::BC_DT_I_64;
		m_exeThreadData.regDatatypes[MarC::BC_MEM_REG_TEMPORARY_DATA] = MarC::BC_DT_UNKNOWN;
		m_exeThreadData.regDatatypes[MarC::BC_MEM_REG_EXIT_CODE] = MarC::BC_DT_I_64;

		m_wndBase = createDebugWindow(1, 1);
		(*m_wndBase)->getSubWnd<Console::SplitWindow>(DbgWndName_LeftHalf)->setTop(m_wndDisasm);

		Console::subTextWndInsert(**m_wndBase, DbgWndName_InputView, ">> ", 1, 0);
		Console::subTextWndInsert(**m_wndBase, DbgWndName_ConsoleTitle, "Console:", 1, 0);
		Console::subTextWndInsert(**m_wndBase, DbgWndName_MemoryTitle, "Memory:", 1, 0);
		Console::subTextWndInsert(**m_wndBase, DbgWndName_CallstackTitle, "Callstack:", 1, 0);
	}

	int Debugger::run()
	{
		if (!m_exeInfo)
			return -1;

		std::thread exeThread(&Debugger::exeThreadFunc, this);
		auto consoleDimensions = Console::getDimensions();
		(*m_wndBase)->resize(consoleDimensions.width, consoleDimensions.height);

		auto wndDisasm = (*m_wndBase)->getSubWnd<Console::TextWindow>(DbgWndName_DisasmViewCode);
		if (wndDisasm)
		{
			
		}

		bool closeDebugger = false;

		bool refreshRequested = false;
		std::chrono::high_resolution_clock::time_point lastRefresh;

		while (!closeDebugger && !m_exeThreadData.threadClosed)
		{
			if (Console::charWaiting())
			{
				unsigned char ch = Console::getChar();
				if (!m_wndBase->handleKeyPress(ch))
					m_wndBase->setFocus(DbgWndName_Disasm);
				switch (ch)
				{
				case 'r': // [R]un the code until a stop is requested or the interpreter exits.
				{
					std::lock_guard lock(m_exeThreadData.mtxExeCount);
					m_exeThreadData.exeCount = -1;
					m_exeThreadData.conExeCount.notify_one();
					refreshRequested = true;
					break;
				}
				case 's': // [S]tep
				{
					std::lock_guard lock(m_exeThreadData.mtxExeCount);
					++m_exeThreadData.exeCount;
					m_exeThreadData.conExeCount.notify_one();
					refreshRequested = true;
					break;
				}
				case 'b': // [B]reak
				{
					std::lock_guard lock(m_exeThreadData.mtxExeCount);
					m_exeThreadData.exeCount = 0;
					m_exeThreadData.conExeCount.notify_one();
					refreshRequested = true;
					break;
				}
				case 'p': // Set/unset breakpoint
				{
					break;
				}
				case 'e': // [E]xit the debugger
					closeDebugger = true;
					break;
				}
			}

			auto newCD = Console::getDimensions();
			if (newCD.width != consoleDimensions.width || newCD.height != consoleDimensions.height)
			{
				consoleDimensions = newCD;
				(*m_wndBase)->resize(consoleDimensions.width, consoleDimensions.height);
				refreshRequested = true;
			}

			auto now = std::chrono::high_resolution_clock::now();
			auto diff = now - lastRefresh;
			if (100000000 < diff.count() || refreshRequested)
			{
				bool updateIsSafe = false;
				if (m_settings.flags.hasFlag(CmdFlags::ForceRefresh))
				{
					m_exeThreadData.mtxExeCount.lock();
					updateIsSafe = true;
				}
				else
				{
					if (m_exeThreadData.mtxExeCount.try_lock())
					{
						if (m_exeThreadData.exeCount > 0)
							m_exeThreadData.mtxExeCount.unlock();
						else
							updateIsSafe = true;
					}
				}

				if (updateIsSafe)
				{
					{
						m_wndDisasm->refresh();

						{
							auto wndMemoryView = (*m_wndBase)->getSubWnd<Console::TextWindow>(DbgWndName_MemoryView);
							int line = 0;
							for (auto reg = MarC::BC_MEM_REG_CODE_POINTER; reg < MarC::BC_MEM_REG_NUM_OF_REGS; reg = (MarC::BC_MemRegister)(reg + 1))
							{
								auto mc = m_interpreter->getRegister(reg);
								auto dt = m_exeThreadData.regDatatypes[reg];
								std::string name = "$" + MarC::BC_RegisterToString(reg);
								name.resize(m_maxPrintSymLen + 1, ' ');
								std::string dtStr = MarC::BC_DatatypeToString(dt);
								dtStr.resize(10, ' ');
								std::string valStr = MarC::BC_MemCellToString(mc, dt);

								std::string lineStr = name + dtStr + valStr;
								wndMemoryView->replace(lineStr, 1, line);
								++line;
							}

							for (auto& sym : m_exeInfo->symbols)
							{
								if (sym.usage == MarC::SymbolUsage::Address &&
									sym.value.as_ADDR.base != MarC::BC_MEM_BASE_CODE_MEMORY &&
									sym.value.as_ADDR.base != MarC::BC_MEM_BASE_REGISTER
									)
								{
									MarC::BC_MemCell mc = m_interpreter->hostMemCell(sym.value.as_ADDR, false);
									auto dt = MarC::BC_DT_I_64; // TODO: Get actual datatype
									std::string name = sym.name;
									name.resize(m_maxPrintSymLen + 1, ' ');
									std::string dtStr = MarC::BC_DatatypeToString(dt);
									dtStr.resize(10, ' ');
									std::string valStr = MarC::BC_MemCellToString(mc, dt);

									std::string lineStr = name + dtStr + valStr;
									wndMemoryView->replace(lineStr, 1, line);
									++line;
								}
							}
						}
					}
					m_exeThreadData.mtxExeCount.unlock();
				}

				std::cout << Console::CurVis::Hide;
				(*m_wndBase)->render(0, 0);
				std::cout << Console::CurVis::Show;
				lastRefresh = now;
				refreshRequested = false;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		m_exeThreadData.stopExecution = true;
		m_exeThreadData.conExeCount.notify_all();
		exeThread.join();

		if (!m_interpreter->lastError().isOK())
		{
			std::cout << "An error occured while running the interpreter!:" << std::endl
				<< m_interpreter->lastError().getMessage() << std::endl;
			return -1;
		}

		int64_t exitCode = m_interpreter->getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64;
		return (int)exitCode;
	}

	void Debugger::exeThreadFunc()
	{
		auto& etd = m_exeThreadData;
		auto& regCP = m_interpreter->getRegister(MarC::BC_MEM_REG_CODE_POINTER);
		while (!m_interpreter->lastError() && !etd.stopExecution)
		{
			std::unique_lock lock(etd.mtxExeCount);
			etd.conExeCount.wait(lock, [&]() { return etd.exeCount != 0 || etd.stopExecution; });

			bool isFirstInstruction = true;
			while (etd.exeCount > 0 && !m_interpreter->lastError() && !etd.stopExecution)
			{
				if (!isFirstInstruction)
				{
					//auto& breakpoints = (*etd.pModInfos)[regCP.as_ADDR.asCode.page].breakpoints;
					//if (breakpoints.find(regCP.as_ADDR) != breakpoints.end())
					//{
					//	etd.exeCount = 0;
					//	lock.unlock();
					//	break;
					//}
				}
				--etd.exeCount;
				lock.unlock();

				m_interpreter->interpret(1);

				lock.lock();

				isFirstInstruction = false;
			}
			etd.exeCount = 0;

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		etd.threadClosed = true;
	}
}
