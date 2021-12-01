#include "Debugger/Debugger.h"

#include <iostream>
#include <thread>

#include "MarCmdExeInfoLoader.h"
#include "PermissionGrantPrompt.h"

namespace MarCmd
{
	DisasmWindow::DisasmWindow(const std::string& name, SharedDebugDataRef sdd, uint64_t modIndex)
		: Console::SplitWindow(name)
	{
		m_sdd = sdd;
		m_modIndex = modIndex;

		auto& mod = m_sdd->interpreter->getExeInfo()->modules[modIndex];
		auto& mem = mod->codeMemory;

		uint64_t nDisassembled = 0;
		while (nDisassembled < mem->size())
		{
			ModDisasmInfo::InsInfo insInfo;
			insInfo.data = MarC::Disassembler::disassemble((char*)mem->getBaseAddress() + nDisassembled);
			insInfo.str = MarC::DisAsmInsInfoToString(insInfo.data, m_sdd->interpreter->getExeInfo()->symbols);
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
		case 'm':
			break;
		case 'u':
			break;
		case 'd':
			break;
		case 's':
		{
			std::lock_guard lock(m_sdd->mtxExeCount);
			m_sdd->exeCount = 1;
			m_sdd->conExeCount.notify_one();
			m_sdd->refreshRequested = true;
			break;
		}
		case 'r':
		{
			std::lock_guard lock(m_sdd->mtxExeCount);
			m_sdd->exeCount = -1;
			m_sdd->conExeCount.notify_one();
			m_sdd->refreshRequested = true;
			break;
		}
		case 'b':
		{
			std::lock_guard lock(m_sdd->mtxExeCount);
			m_sdd->exeCount = 0;
			m_sdd->conExeCount.notify_one();
			m_sdd->refreshRequested = true;
			break;
		}
		case 'x':
		{
			std::lock_guard lock(m_sdd->mtxExeCount);
			if (!m_sdd->exeCount)
			{
				toggleBreakpoint(m_sdd->interpreter->getRegister(MarC::BC_MEM_REG_CODE_POINTER).as_ADDR);
			}
			break;
		}
		default:
			Window::handleKeyPress(key);
		}

	}

	uint64_t DisasmWindow::getModIndex() const
	{
		return m_modIndex;
	}

	void DisasmWindow::refresh()
	{
		MarC::BC_MemAddress exeAddr = m_sdd->interpreter->getRegister(MarC::BC_MEM_REG_CODE_POINTER).as_ADDR;
		uint64_t modIndex = exeAddr.asCode.page;
		uint64_t offset = exeAddr.asCode.addr;
		int64_t line = MarC::searchBinary(offset, m_modDisasmInfo.instructionOffsets);
		{
			auto wndDisasmTitle = this->getSubWnd<Console::TextWindow>("Title");
			auto wndDisasmViewControlInsPtr = this->getSubWnd<Console::TextWindow>("Line Marker");
			auto wndDisasmViewControlBreakpoints = this->getSubWnd<Console::TextWindow>("Breakpoints");
			auto wndDisasmViewCode = this->getSubWnd<Console::TextWindow>("Code");
			wndDisasmTitle->replace("Disassembly: " + m_sdd->interpreter->getExeInfo()->modules[modIndex]->moduleName, 1, 0);
			int64_t mid = wndDisasmViewCode->getHeight() / 2;
			wndDisasmViewControlInsPtr->setScroll(-mid);
			wndDisasmViewControlBreakpoints->setScroll(-mid);
			wndDisasmViewCode->setScroll(line - mid);
		}
	}

	bool DisasmWindow::hasBreakpoint(MarC::BC_MemAddress breakpoint)
	{
		std::lock_guard lock(m_modDisasmInfo.mtxBreakpoints);
		return m_modDisasmInfo.breakpoints.find(breakpoint) != m_modDisasmInfo.breakpoints.end();
	}

	bool DisasmWindow::toggleBreakpoint(MarC::BC_MemAddress breakpoint)
	{
		if (hasBreakpoint(breakpoint))
		{
			m_modDisasmInfo.breakpoints.erase(breakpoint);
			return false;
		}
		m_modDisasmInfo.breakpoints.insert(breakpoint);
		return true;
	}

	DisasmWindowRef DisasmWindow::create(const std::string& name, SharedDebugDataRef sdd, uint64_t modIndex)
	{
		auto temp = std::shared_ptr<DisasmWindow>(new DisasmWindow(name, sdd, modIndex));
		temp->setSelfRef(temp);
		return temp;
	}

	int Debugger::run(const Settings& settings)
	{
		auto dbgr = Debugger(settings);
		return dbgr.run();
	}

	Debugger::Debugger(const Settings& settings)
		: m_settings(settings)
	{
		m_sharedDebugData = std::make_shared<SharedDebugData>();
		m_sharedDebugData->exeInfo = loadExeInfo(m_settings);

		for (auto& sym : m_sharedDebugData->exeInfo->symbols)
		{
			if (sym.usage == MarC::SymbolUsage::Address &&
				sym.value.as_ADDR.base != MarC::BC_MEM_BASE_CODE_MEMORY &&
				sym.value.as_ADDR.base != MarC::BC_MEM_BASE_REGISTER
				)
				m_maxPrintSymLen = std::max(m_maxPrintSymLen, sym.name.size());
		}

		m_sharedDebugData->interpreter = MarC::Interpreter::create(m_sharedDebugData->exeInfo);
		for (auto& entry : settings.extDirs)
			m_sharedDebugData->interpreter->addExtDir(entry);

		m_vecWndDisasm.resize(m_sharedDebugData->exeInfo->modules.size());
		for (uint64_t i = 0; i < m_sharedDebugData->exeInfo->modules.size(); ++i)
			m_vecWndDisasm[i] = DisasmWindow::create("Disassembly", m_sharedDebugData, i);
		m_wndDisasm = m_vecWndDisasm[0];

		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_CODE_POINTER] = MarC::BC_DT_ADDR;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_STACK_POINTER] = MarC::BC_DT_ADDR;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_FRAME_POINTER] = MarC::BC_DT_ADDR;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_LOOP_COUNTER] = MarC::BC_DT_UNKNOWN;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_ACCUMULATOR] = MarC::BC_DT_I_64;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_TEMPORARY_DATA] = MarC::BC_DT_UNKNOWN;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_EXIT_CODE] = MarC::BC_DT_I_64;

		m_wndBase = createDebugWindow(1, 1);
		(*m_wndBase)->getSubWnd<Console::SplitWindow>(DbgWndName_LeftHalf)->setTop(m_wndDisasm);

		Console::subTextWndInsert(**m_wndBase, DbgWndName_InputView, ">> ", 1, 0);
		Console::subTextWndInsert(**m_wndBase, DbgWndName_ConsoleTitle, "Console:", 1, 0);
		Console::subTextWndInsert(**m_wndBase, DbgWndName_MemoryTitle, "Memory:", 1, 0);
		Console::subTextWndInsert(**m_wndBase, DbgWndName_CallstackTitle, "Callstack:", 1, 0);
	}

	int Debugger::run()
	{
		if (!m_sharedDebugData->exeInfo)
			return -1;

		if (m_sharedDebugData->interpreter->hasUngrantedPerms())
		{
			if (m_settings.flags.hasFlag(CmdFlags::GrantAll))
			{
				m_sharedDebugData->interpreter->grantAllPerms();
			}
			else
			{
				std::set<std::string> toGrant;

				auto manPerms = m_sharedDebugData->interpreter->getUngrantedPerms(m_sharedDebugData->interpreter->getManPerms());
				if (!manPerms.empty())
					permissionGrantPrompt(PermissionPromptType::Mandatory, manPerms, toGrant);

				auto optPerms = m_sharedDebugData->interpreter->getUngrantedPerms(m_sharedDebugData->interpreter->getOptPerms());
				if (!optPerms.empty())
					permissionGrantPrompt(PermissionPromptType::Optional, optPerms, toGrant);

				m_sharedDebugData->interpreter->grantPerms(toGrant);
			}
		}

		std::thread exeThread(&Debugger::exeThreadFunc, this);
		auto consoleDimensions = Console::getDimensions();
		(*m_wndBase)->resize(consoleDimensions.width, consoleDimensions.height);

		bool closeDebugger = false;

		std::chrono::high_resolution_clock::time_point lastRefresh;

		while (!closeDebugger && !m_sharedDebugData->threadClosed)
		{
			if (Console::charWaiting())
			{
				unsigned char ch = Console::getChar();
				switch (ch)
				{
				case 'E': // [E]xit the debugger
					closeDebugger = true;
					break;
				default:
					if (!m_wndBase->handleKeyPress(ch))
						m_wndBase->setFocus("Disassembly");
				}
			}

			auto newCD = Console::getDimensions();
			if (newCD.width != consoleDimensions.width || newCD.height != consoleDimensions.height)
			{
				consoleDimensions = newCD;
				(*m_wndBase)->resize(consoleDimensions.width, consoleDimensions.height);
				m_sharedDebugData->refreshRequested = true;
			}

			auto now = std::chrono::high_resolution_clock::now();
			auto diff = now - lastRefresh;
			if (100000000 < diff.count() || m_sharedDebugData->refreshRequested)
			{
				bool updateIsSafe = false;
				if (m_settings.flags.hasFlag(CmdFlags::ForceRefresh))
				{
					m_sharedDebugData->mtxExeCount.lock();
					updateIsSafe = true;
				}
				else
				{
					if (m_sharedDebugData->mtxExeCount.try_lock())
					{
						if (m_sharedDebugData->exeCount > 0)
							m_sharedDebugData->mtxExeCount.unlock();
						else
							updateIsSafe = true;
					}
				}

				if (updateIsSafe)
				{
					{
						MarC::BC_MemAddress cp = m_sharedDebugData->interpreter->getRegister(MarC::BC_MEM_REG_CODE_POINTER).as_ADDR;
						if (cp.asCode.page != m_wndDisasm->getModIndex())
						{
							bool hadFocus = m_wndBase->getFocus() == m_wndDisasm;
							m_wndDisasm = m_vecWndDisasm[cp.asCode.page];
							(*m_wndBase)->replaceSubWnd("Disassembly", m_wndDisasm);
							if (hadFocus)
								m_wndBase->setFocus("Disassembly");
						}

						m_wndDisasm->refresh();

						{
							auto wndMemoryView = (*m_wndBase)->getSubWnd<Console::TextWindow>(DbgWndName_MemoryView);
							int line = 0;
							for (auto reg = MarC::BC_MEM_REG_CODE_POINTER; reg < MarC::BC_MEM_REG_NUM_OF_REGS; reg = (MarC::BC_MemRegister)(reg + 1))
							{
								auto mc = m_sharedDebugData->interpreter->getRegister(reg);
								auto dt = m_sharedDebugData->regDatatypes[reg];
								std::string name = "$" + MarC::BC_RegisterToString(reg);
								name.resize(m_maxPrintSymLen + 1, ' ');
								std::string dtStr = MarC::BC_DatatypeToString(dt);
								dtStr.resize(10, ' ');
								std::string valStr = MarC::BC_MemCellToString(mc, dt);

								std::string lineStr = name + dtStr + valStr;
								wndMemoryView->replace(lineStr, 1, line);
								++line;
							}

							for (auto& sym : m_sharedDebugData->exeInfo->symbols)
							{
								if (sym.usage == MarC::SymbolUsage::Address &&
									sym.value.as_ADDR.base != MarC::BC_MEM_BASE_CODE_MEMORY &&
									sym.value.as_ADDR.base != MarC::BC_MEM_BASE_REGISTER
									)
								{
									MarC::BC_MemCell mc = m_sharedDebugData->interpreter->hostMemCell(sym.value.as_ADDR, false);
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
					m_sharedDebugData->mtxExeCount.unlock();
				}

				std::cout << Console::CurVis::Hide;
				(*m_wndBase)->render(0, 0);
				std::cout << Console::CurVis::Show;
				lastRefresh = now;
				m_sharedDebugData->refreshRequested = false;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		m_sharedDebugData->stopExecution = true;
		m_sharedDebugData->conExeCount.notify_all();
		exeThread.join();

		if (!m_sharedDebugData->interpreter->lastError().isOK())
		{
			std::cout << "An error occured while running the interpreter!:" << std::endl
				<< m_sharedDebugData->interpreter->lastError().getMessage() << std::endl;
			return -1;
		}

		int64_t exitCode = m_sharedDebugData->interpreter->getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64;
		return (int)exitCode;
	}

	void Debugger::exeThreadFunc()
	{
		auto& sdd = m_sharedDebugData;
		auto& regCP = m_sharedDebugData->interpreter->getRegister(MarC::BC_MEM_REG_CODE_POINTER);
		while (!m_sharedDebugData->interpreter->lastError() && !sdd->stopExecution)
		{
			{
				std::unique_lock lock(sdd->mtxExeCount);
				sdd->conExeCount.wait(lock, [&]() { return sdd->exeCount != 0 || sdd->stopExecution; });

				bool ignoreBreakpoint = true;
				while (sdd->exeCount > 0 && !m_sharedDebugData->interpreter->lastError() && !sdd->stopExecution)
				{
					if (!ignoreBreakpoint)
					{
						if (m_wndDisasm->hasBreakpoint(regCP.as_ADDR))
						{
							sdd->exeCount = 0;
							lock.unlock();
							break;
						}
					}
					--sdd->exeCount;
					lock.unlock();

					m_sharedDebugData->interpreter->interpret(1);

					lock.lock();

					ignoreBreakpoint = false;
				}
				sdd->exeCount = 0;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		sdd->threadClosed = true;
	}
}