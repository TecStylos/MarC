#include "Debugger/Debugger.h"

#include <iostream>
#include <mutex>
#include <thread>

#include "PermissionGrantPrompt.h"
#include "AutoExecutableLoader.h"
#include "errors/MarCoreError.h"
#include "types/BytecodeTypes.h"
#include "types/DisAsmTypes.h"

namespace MarCmd
{
	DisasmWindow::DisasmWindow(const std::string& name, SharedDebugDataRef sdd, uint64_t modIndex)
		: Console::SplitWindow(name)
	{
		m_sdd = sdd;
		m_modIndex = modIndex;

		auto& mem = m_sdd->interpreter->getExeInfo()->codeMemory;

		uint64_t nDisassembled = 0;
		while (nDisassembled < mem.size())
		{
			ModDisasmInfo::InsInfo insInfo;
			insInfo.data = MarC::Disassembler::disassemble((char*)mem.getBaseAddress() + nDisassembled);
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
			wndDisasmViewSplit->setRatio(Console::WRT::AbsoluteLeft, 4);
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
		case 'u': // Scroll up
			--m_scrollOffset;
			m_sdd->refreshRequested = true;
			break;
		case 'd': // Scroll down
			++m_scrollOffset;
			m_sdd->refreshRequested = true;
			break;
		case 's': // Step into
		{
			std::lock_guard lock(m_sdd->mtxExeCount);
			m_sdd->exeCount = 1;
			m_sdd->conExeCount.notify_one();
			m_sdd->refreshRequested = true;
			break;
		}
		case 'r': // Run
		{
			std::lock_guard lock(m_sdd->mtxExeCount);
			m_sdd->exeCount = -1;
			m_sdd->conExeCount.notify_one();
			m_sdd->refreshRequested = true;
			break;
		}
		case 'b': // Break
		{
			std::lock_guard lock(m_sdd->mtxExeCount);
			m_sdd->exeCount = 0;
			m_sdd->conExeCount.notify_one();
			m_sdd->refreshRequested = true;
			break;
		}
		case 'x': // Set breakpoint
		{
			std::lock_guard lock(m_sdd->mtxExeCount);
			if (!m_sdd->exeCount)
			{
				auto wndDisasmViewCode = this->getSubWnd<Console::TextWindow>("Code");
				int64_t mid = wndDisasmViewCode->getHeight() / 2;
				int64_t line = mid + wndDisasmViewCode->getScroll();
				if (0 <= line && line < (int64_t)m_modDisasmInfo.instructionOffsets.size())
					toggleBreakpoint(MarC::BC_MemAddress(MarC::BC_MEM_BASE_CODE_MEMORY, m_modDisasmInfo.instructionOffsets[line]));
			}
			break;
		}
		default:
			Window::handleKeyPress(key);
		}
	}

	void DisasmWindow::refresh()
	{
		MarC::BC_MemAddress exeAddr = m_sdd->interpreter->getRegister(MarC::BC_MEM_REG_CODE_POINTER).as_ADDR;

		auto wndDisasmTitle = this->getSubWnd<Console::TextWindow>("Title");
		auto wndDisasmViewControlInsPtr = this->getSubWnd<Console::TextWindow>("Line Marker");
		auto wndDisasmViewControlBreakpoints = this->getSubWnd<Console::TextWindow>("Breakpoints");
		auto wndDisasmViewCode = this->getSubWnd<Console::TextWindow>("Code");

		wndDisasmTitle->replace("Disassembly: " + m_sdd->interpreter->getExeInfo()->name, 1, 0);

		int64_t exeLine = m_modDisasmInfo.addrToLine(exeAddr);
		if (m_nInsExecuted != m_sdd->interpreter->nInsExecuted())
		{
			m_scrollOffset = 0;
			wndDisasmViewControlInsPtr->replace("=>", 0, 0);
			m_nInsExecuted = m_sdd->interpreter->nInsExecuted();
		}
		else if (m_scrollOffset == 0)
		{
			wndDisasmViewControlInsPtr->replace("=>", 0, 0);
		}
		else
		{
			wndDisasmViewControlInsPtr->replace("->", 0, 0);
		}

		int64_t mid = -(int64_t)wndDisasmViewCode->getHeight() / 2;
		wndDisasmViewControlInsPtr->setScroll(mid);
		wndDisasmViewControlBreakpoints->setScroll(mid + exeLine + m_scrollOffset);
		wndDisasmViewCode->setScroll(mid + exeLine + m_scrollOffset);
	}

	bool DisasmWindow::hasBreakpoint(MarC::BC_MemAddress breakpoint)
	{
		std::lock_guard lock(m_modDisasmInfo.mtxBreakpoints);
		return m_modDisasmInfo.breakpoints.find(breakpoint) != m_modDisasmInfo.breakpoints.end();
	}

	bool DisasmWindow::toggleBreakpoint(MarC::BC_MemAddress breakpoint)
	{
		int64_t line = m_modDisasmInfo.addrToLine(breakpoint);

		if (hasBreakpoint(breakpoint))
		{
			m_modDisasmInfo.breakpoints.erase(breakpoint);
			getSubWnd<Console::TextWindow>("Breakpoints")->replace(" ", 0, line);
			return false;
		}

		m_modDisasmInfo.breakpoints.insert(breakpoint);
		getSubWnd<Console::TextWindow>("Breakpoints")->replace("+", 0, line);
		return true;
	}

	const DisasmWindow::ModDisasmInfo& DisasmWindow::getDisasmInfo() const
	{
		return m_modDisasmInfo;
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

		m_sharedDebugData->exeInfo = autoLoadExecutable(settings.inFile, settings.modDirs);

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

		m_vecWndDisasm.resize(1);
			m_vecWndDisasm[0] = DisasmWindow::create("Disassembly", m_sharedDebugData, 0);
		m_wndDisasm = m_vecWndDisasm[0];

		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_CODE_POINTER / sizeof(uint64_t)] = MarC::BC_DT_ADDR;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_STACK_POINTER / sizeof(uint64_t)] = MarC::BC_DT_ADDR;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_FRAME_POINTER / sizeof(uint64_t)] = MarC::BC_DT_ADDR;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_LOOP_COUNTER / sizeof(uint64_t)] = MarC::BC_DT_UNKNOWN;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_ACCUMULATOR / sizeof(uint64_t)] = MarC::BC_DT_I_64;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_TEMPORARY_DATA / sizeof(uint64_t)] = MarC::BC_DT_UNKNOWN;
		m_sharedDebugData->regDatatypes[MarC::BC_MEM_REG_EXIT_CODE / sizeof(uint64_t)] = MarC::BC_DT_I_64;

		m_sharedDebugData->wndBase = createDebugWindow(1, 1);
		(*m_sharedDebugData->wndBase)->getSubWnd<Console::SplitWindow>(DbgWndName_LeftHalf)->setTop(m_wndDisasm);
		m_sharedDebugData->wndBase->setFocus("Disassembly");

		m_wndCallstack = (*m_sharedDebugData->wndBase)->getSubWnd<Console::TextWindow>(DbgWndName_CallstackView);

		Console::subTextWndInsert(**m_sharedDebugData->wndBase, DbgWndName_InputView, ">> ", 1, 0);
		Console::subTextWndInsert(**m_sharedDebugData->wndBase, DbgWndName_ConsoleTitle, "Console:", 1, 0);
		Console::subTextWndInsert(**m_sharedDebugData->wndBase, DbgWndName_MemoryTitle, "Memory:", 1, 0);
		Console::subTextWndInsert(**m_sharedDebugData->wndBase, DbgWndName_ModuleTitle, "Module Browser:", 1, 0);
		Console::subTextWndInsert(**m_sharedDebugData->wndBase, DbgWndName_CallstackTitle, "Callstack:", 1, 0);
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
		(*m_sharedDebugData->wndBase)->resize(consoleDimensions.width, consoleDimensions.height);

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
					if (!m_sharedDebugData->wndBase->handleKeyPress(ch))
						m_sharedDebugData->wndBase->setFocus("Disassembly");
				}
			}

			auto newCD = Console::getDimensions();
			if (newCD.width != consoleDimensions.width || newCD.height != consoleDimensions.height)
			{
				consoleDimensions = newCD;
				(*m_sharedDebugData->wndBase)->resize(consoleDimensions.width, consoleDimensions.height);
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
					updateDisasm();
					updateMemoryView();
					updateCallstack();

					m_sharedDebugData->mtxExeCount.unlock();
				}

				std::cout << Console::CurVis::Hide;
				(*m_sharedDebugData->wndBase)->render(0, 0);
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
				<< m_sharedDebugData->interpreter->lastError().what() << std::endl;
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

					// Get information about the next instruction to execute
					const auto& insInfo = m_wndDisasm->getDisasmInfo().ins[m_wndDisasm->getDisasmInfo().addrToLine(regCP.as_ADDR)].data;

					m_sharedDebugData->interpreter->interpret(1);

					// Update symbol datatype;
					if (false)
					{
						const auto& addrArgIndices = std::vector<uint64_t>();
						std::string currScopeName;
						if (!m_sharedDebugData->callstack.empty())
						{
							auto it = MarC::getSymbolForAddress(m_sharedDebugData->callstack.back(), m_sharedDebugData->interpreter->getExeInfo()->symbols);
							if (it != m_sharedDebugData->interpreter->getExeInfo()->symbols.end())
								currScopeName = it->name;
						}
						for (uint64_t index : addrArgIndices)
						{
							auto& arg = insInfo.args[index];
							if (arg.value.datatype == MarC::BC_DT_ADDR)
							{
								auto it = MarC::getSymbolForAddress(arg.value.cell.as_ADDR, m_sharedDebugData->interpreter->getExeInfo()->symbols, currScopeName);
								if (it == m_sharedDebugData->interpreter->getExeInfo()->symbols.end())
									continue;
								m_sharedDebugData->symbolTypes[it->name] = arg.value.datatype;
							}
						}
					}

					switch(insInfo.ocx.opCode)
					{
					case MarC::BC_OC_CALL:
					{
						std::lock_guard lock(m_sharedDebugData->mtxCallstack);
						++m_sharedDebugData->callstackModifyCount;
						MarC::BC_MemAddress funcAddr = insInfo.args[0].value.cell.as_ADDR;
						for (MarC::DerefCount i = 0; i < insInfo.args[0].derefCount; ++i)
							funcAddr = m_sharedDebugData->interpreter->hostMemCell(funcAddr).as_ADDR;
						m_sharedDebugData->callstack.push_back(funcAddr);
						break;
					}
					case MarC::BC_OC_RETURN:
					{
						std::lock_guard lock(m_sharedDebugData->mtxCallstack);
						++m_sharedDebugData->callstackModifyCount;
						m_sharedDebugData->callstack.pop_back();
						break;
					}
					default:
						;
					}

					lock.lock();

					ignoreBreakpoint = false;
				}
				sdd->exeCount = 0;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		sdd->threadClosed = true;
	}

	void Debugger::updateDisasm()
	{
		m_wndDisasm->refresh();
	}

	void Debugger::updateMemoryView()
	{
		std::string currCallstackName;
		if (!m_sharedDebugData->callstack.empty())
		{
			auto it = MarC::getSymbolForAddress(
				m_sharedDebugData->callstack.back(),
				m_sharedDebugData->interpreter->getExeInfo()->symbols
			);
			if (it != m_sharedDebugData->interpreter->getExeInfo()->symbols.end())
				currCallstackName = it->name;
		}

		auto wndMemoryView = (*m_sharedDebugData->wndBase)->getSubWnd<Console::TextWindow>(DbgWndName_MemoryView);
		wndMemoryView->clearText();
		
		for (auto reg = MarC::BC_MEM_REG_CODE_POINTER; reg < MarC::_BC_MEM_REG_NUM * sizeof(uint64_t); reg = (MarC::BC_MemRegister)(reg + sizeof(uint64_t)))
		{
			auto mc = m_sharedDebugData->interpreter->getRegister(reg);
			auto dt = m_sharedDebugData->regDatatypes[reg / sizeof(uint64_t)];
			std::string name = "$" + MarC::BC_RegisterToString(reg);
			name.resize(m_maxPrintSymLen + 1, ' ');
			std::string dtStr = MarC::BC_DatatypeToString(dt);
			dtStr.resize(10, ' ');
			std::string valStr = MarC::BC_MemCellToString(mc, dt);

			std::string lineStr = " " + name + dtStr + valStr;
			wndMemoryView->append(lineStr + "\n");
		}

		for (auto& sym : m_sharedDebugData->exeInfo->symbols)
		{
			// Only view symbols from the current scope
			if (sym.name.find(currCallstackName) != 0)
				continue;
			if (sym.usage != MarC::SymbolUsage::Address)
				continue;
			if (sym.value.as_ADDR.base == MarC::BC_MEM_BASE_CODE_MEMORY)
				continue;
			if (sym.value.as_ADDR.base == MarC::BC_MEM_BASE_REGISTER)
				continue;

			MarC::BC_MemCell mc = m_sharedDebugData->interpreter->hostMemCell(sym.value.as_ADDR, false);
			auto dt = MarC::BC_DT_I_64; // TODO: Get actual datatype
			std::string name = sym.name;
			name.resize(m_maxPrintSymLen + 1, ' ');
			std::string dtStr = MarC::BC_DatatypeToString(dt);
			dtStr.resize(10, ' ');
			std::string valStr = MarC::BC_MemCellToString(mc, dt);

			std::string lineStr = " " + name + dtStr + valStr;
			wndMemoryView->append(lineStr + "\n");
		}
	}

	void Debugger::updateCallstack()
	{
		std::lock_guard lock(m_sharedDebugData->mtxCallstack);

		bool needsScrollUpdate = false;
		if (m_callstackLastModCount != m_sharedDebugData->callstackModifyCount)
		{
			m_callstackLastModCount = m_sharedDebugData->callstackModifyCount;

			m_wndCallstack->clearText();

			uint64_t csSize = m_sharedDebugData->callstack.size();
			uint64_t lineNrWidth = std::to_string(csSize).size();

			std::string globNr = "0";
			globNr.resize(lineNrWidth, ' ');

			m_wndCallstack->append((csSize ? "   [" : "-> [") + globNr + "] <global>\n");

			for (uint64_t i = 0; i < csSize; ++i)
			{
				MarC::BC_MemAddress addr = m_sharedDebugData->callstack[i];

				auto it = MarC::getSymbolForAddress(addr, m_sharedDebugData->interpreter->getExeInfo()->symbols);
				const char* pfix = (i + 1 == csSize) ? "-> [" : "   [";
				std::string lineNr = std::to_string(i + 1);
				lineNr.resize(lineNrWidth, ' ');
				if (it != m_sharedDebugData->interpreter->getExeInfo()->symbols.end())
					m_wndCallstack->append(pfix + lineNr + "] " + it->name + "\n");
				else
				 	m_wndCallstack->append(pfix + lineNr + "] " + MarC::BC_MemAddressToString(addr) + "\n");
			}
			needsScrollUpdate = true;
		}

		if (m_callstackLastHeight != m_wndCallstack->getHeight() || needsScrollUpdate)
		{
			m_callstackLastHeight = m_wndCallstack->getHeight();
			int64_t csSize = m_sharedDebugData->callstack.size();

			int64_t scroll = 0;
			if (csSize >= m_callstackLastHeight)
				scroll = csSize - m_callstackLastHeight + 1;

			m_wndCallstack->setScroll(scroll);
		}
	}
}