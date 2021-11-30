#include "Debugger/Debugger.h"

#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>

#include <MarCore.h>

#include "Debugger/DebugWindow.h"
#include "MarCmdExeInfoLoader.h"

namespace MarCmd
{
	int Debugger::run(const Settings& settings)
	{
		MarC::ExecutableInfoRef exeInfo = loadExeInfo(settings);
		if (!exeInfo)
			return -1;

		uint64_t maxPrintSymLen = 0;
		for (auto& sym : exeInfo->symbols)
		{
			if (sym.usage == MarC::SymbolUsage::Address &&
				sym.value.as_ADDR.base != MarC::BC_MEM_BASE_CODE_MEMORY &&
				sym.value.as_ADDR.base != MarC::BC_MEM_BASE_REGISTER
				)
				maxPrintSymLen = std::max(maxPrintSymLen, sym.name.size());
		}

		struct ModuleDisasmInfo
		{
			std::vector<uint64_t> instructionOffsets;
			struct InsInfo
			{
				std::string str;
				MarC::DisAsmInsInfo data;
			};
			std::vector<InsInfo> ins;
			std::set<MarC::BC_MemAddress> breakpoints;
		};

		std::vector<ModuleDisasmInfo> modDisasmInfo(exeInfo->modules.size());
		for (uint64_t modIndex = 0; modIndex < exeInfo->modules.size(); ++modIndex)
		{
			auto& inf = modDisasmInfo[modIndex];
			auto& mod = exeInfo->modules[modIndex];
			auto& mem = mod->codeMemory;

			uint64_t nDisassembled = 0;
			while (nDisassembled < mem->size())
			{
				ModuleDisasmInfo::InsInfo insInfo;
				insInfo.data = MarC::Disassembler::disassemble((char*)mem->getBaseAddress() + nDisassembled);
				insInfo.str = MarC::DisAsmInsInfoToString(insInfo.data, exeInfo->symbols);
				inf.ins.push_back(insInfo);
				inf.instructionOffsets.push_back(nDisassembled);
				nDisassembled += inf.ins.back().data.rawData.size();
			}
		}

		struct ExeThreadData
		{
			std::shared_ptr<MarC::Interpreter> pInterpreter = nullptr;
			std::vector<ModuleDisasmInfo>* pModInfos;

			uint64_t exeCount = 0;
			std::mutex mtxExeCount;
			std::condition_variable conExeCount;

			std::atomic_bool stopExecution = false;
			std::atomic_bool threadClosed = false;
			MarC::BC_Datatype regDatatypes[MarC::BC_MEM_REG_NUM_OF_REGS];
		} exeThreadData;
		exeThreadData.pInterpreter = std::make_shared<MarC::Interpreter>(exeInfo);
		exeThreadData.pModInfos = &modDisasmInfo;
		exeThreadData.regDatatypes[MarC::BC_MEM_REG_CODE_POINTER] = MarC::BC_DT_ADDR;
		exeThreadData.regDatatypes[MarC::BC_MEM_REG_STACK_POINTER] = MarC::BC_DT_ADDR;
		exeThreadData.regDatatypes[MarC::BC_MEM_REG_FRAME_POINTER] = MarC::BC_DT_ADDR;
		exeThreadData.regDatatypes[MarC::BC_MEM_REG_LOOP_COUNTER] = MarC::BC_DT_UNKNOWN;
		exeThreadData.regDatatypes[MarC::BC_MEM_REG_ACCUMULATOR] = MarC::BC_DT_I_64;
		exeThreadData.regDatatypes[MarC::BC_MEM_REG_TEMPORARY_DATA] = MarC::BC_DT_UNKNOWN;
		exeThreadData.regDatatypes[MarC::BC_MEM_REG_EXIT_CODE] = MarC::BC_DT_I_64;

		std::thread exeThread(
			[](ExeThreadData* pEtd)
			{
				auto& etd = *pEtd;
				auto& regCP = etd.pInterpreter->getRegister(MarC::BC_MEM_REG_CODE_POINTER);
				while (!etd.pInterpreter->lastError() && !etd.stopExecution)
				{
					std::unique_lock lock(etd.mtxExeCount);
					etd.conExeCount.wait(lock, [&]() { return etd.exeCount != 0 || etd.stopExecution; });

					bool isFirstInstruction = true;
					while (etd.exeCount > 0 && !etd.pInterpreter->lastError() && !etd.stopExecution)
					{
						if (!isFirstInstruction)
						{
							auto& breakpoints = (*etd.pModInfos)[regCP.as_ADDR.asCode.page].breakpoints;
							if (breakpoints.find(regCP.as_ADDR) != breakpoints.end())
							{
								etd.exeCount = 0;
								lock.unlock();
								break;
							}
						}
						--etd.exeCount;
						lock.unlock();

						etd.pInterpreter->interpret(1);

						lock.lock();

						isFirstInstruction = false;
					}
					etd.exeCount = 0;

					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
				etd.threadClosed = true;
			},
			&exeThreadData
		);

		auto consoleDimensions = Console::getDimensions();
		auto wndFull = createDebugWindow(consoleDimensions.width, consoleDimensions.height);

		Console::subTextWndInsert(*wndFull, DbgWndName_DisasmTitle, "Disassembly:", 1, 0);
		Console::subTextWndInsert(*wndFull, DbgWndName_ConsoleTitle, "Console:", 1, 0);
		Console::subTextWndInsert(*wndFull, DbgWndName_InputView, ">> ", 1, 0);
		Console::subTextWndInsert(*wndFull, DbgWndName_MemoryTitle, "Memory:", 1, 0);
		Console::subTextWndInsert(*wndFull, DbgWndName_CallstackTitle, "Callstack:", 1, 0);
		Console::subTextWndInsert(*wndFull, DbgWndName_DisasmViewControlInsPtr, "->", 0, 0);

		auto wndDisasm = wndFull->getSubWnd<Console::TextWindow>(DbgWndName_DisasmViewCode);
		if (wndDisasm)
		{
			for (auto& ins : modDisasmInfo[0].ins)
				wndDisasm->append(ins.str + "\n");
		}

		bool closeDebugger = false;

		bool refreshRequested = false;
		std::chrono::high_resolution_clock::time_point lastRefresh;

		while (!closeDebugger && !exeThreadData.threadClosed)
		{
			if (Console::charWaiting())
			{
				switch (Console::getChar())
				{
				case 'r': // [R]un the code until a stop is requested or the interpreter exits.
				{
					std::lock_guard lock(exeThreadData.mtxExeCount);
					exeThreadData.exeCount = -1;
					exeThreadData.conExeCount.notify_one();
					refreshRequested = true;
					break;
				}
				case 's': // [S]tep
				{
					std::lock_guard lock(exeThreadData.mtxExeCount);
					++exeThreadData.exeCount;
					exeThreadData.conExeCount.notify_one();
					refreshRequested = true;
					break;
				}
				case 'b': // [B]reak
				{
					std::lock_guard lock(exeThreadData.mtxExeCount);
					exeThreadData.exeCount = 0;
					exeThreadData.conExeCount.notify_one();
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
				wndFull->resize(consoleDimensions.width, consoleDimensions.height);
				refreshRequested = true;
			}

			auto now = std::chrono::high_resolution_clock::now();
			auto diff = now - lastRefresh;
			if (100000000 < diff.count() || refreshRequested)
			{
				bool updateIsSafe = false;
				if (settings.flags.hasFlag(CmdFlags::AggressiveRefresh))
				{
					exeThreadData.mtxExeCount.lock();
					updateIsSafe = true;
				}
				else
				{
					if (exeThreadData.mtxExeCount.try_lock())
					{
						if (exeThreadData.exeCount > 0)
							exeThreadData.mtxExeCount.unlock();
						else
							updateIsSafe = true;
					}
				}
				
				if (updateIsSafe)
				{
					{
						MarC::BC_MemAddress exeAddr = exeThreadData.pInterpreter->getRegister(MarC::BC_MEM_REG_CODE_POINTER).as_ADDR;
						uint64_t modIndex = exeAddr.asCode.page;
						uint64_t offset = exeAddr.asCode.addr;
						int64_t line = MarC::searchBinary(offset, modDisasmInfo[modIndex].instructionOffsets);

						{
							auto wndDisasmTitle = wndFull->getSubWnd<Console::TextWindow>(DbgWndName_DisasmTitle);
							auto wndDisasmViewControlInsPtr = wndFull->getSubWnd<Console::TextWindow>(DbgWndName_DisasmViewControlInsPtr);
							auto wndDisasmViewControlBreakpoints = wndFull->getSubWnd<Console::TextWindow>(DbgWndName_DisasmViewControlBreakpoints);
							auto wndDisasmViewCode = wndFull->getSubWnd<Console::TextWindow>(DbgWndName_DisasmViewCode);
							wndDisasmTitle->replace("Disassembly: " + exeInfo->modules[modIndex]->moduleName, 1, 0);
							int64_t mid = wndDisasmViewCode->getHeight() / 2;
							wndDisasmViewControlInsPtr->setScroll(-mid);
							wndDisasmViewControlBreakpoints->setScroll(-mid);
							wndDisasmViewCode->setScroll(-mid + line);
						}

						{
							auto wndMemoryView = wndFull->getSubWnd<Console::TextWindow>(DbgWndName_MemoryView);
							int line = 0;
							for (auto reg = MarC::BC_MEM_REG_CODE_POINTER; reg < MarC::BC_MEM_REG_NUM_OF_REGS; reg = (MarC::BC_MemRegister)(reg + 1))
							{
								auto mc = exeThreadData.pInterpreter->getRegister(reg);
								auto dt = exeThreadData.regDatatypes[reg];
								std::string name = "$" + MarC::BC_RegisterToString(reg);
								name.resize(maxPrintSymLen + 1, ' ');
								std::string dtStr = MarC::BC_DatatypeToString(dt);
								dtStr.resize(10, ' ');
								std::string valStr = MarC::BC_MemCellToString(mc, dt);

								std::string lineStr = name + dtStr + valStr;
								wndMemoryView->replace(lineStr, 1, line);
								++line;
							}

							for (auto& sym : exeInfo->symbols)
							{
								if (sym.usage == MarC::SymbolUsage::Address &&
									sym.value.as_ADDR.base != MarC::BC_MEM_BASE_CODE_MEMORY &&
									sym.value.as_ADDR.base != MarC::BC_MEM_BASE_REGISTER
									)
								{
									auto mc = exeThreadData.pInterpreter->hostMemCell(sym.value.as_ADDR, false);
									auto dt = MarC::BC_DT_I_64; // TODO: Get actual datatype
									std::string name = sym.name;
									name.resize(maxPrintSymLen + 1, ' ');
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
					exeThreadData.mtxExeCount.unlock();
				}

				std::cout << Console::CurVis::Hide;
				wndFull->render(0, 0);
				std::cout << Console::CurVis::Show;
				lastRefresh = now;
				refreshRequested = false;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		exeThreadData.stopExecution = true;
		exeThreadData.conExeCount.notify_all();
		exeThread.join();

		if (!exeThreadData.pInterpreter->lastError().isOK())
		{
			std::cout << "An error occured while running the interpreter!:" << std::endl
				<< exeThreadData.pInterpreter->lastError().getMessage() << std::endl;
			return -1;
		}

		int64_t exitCode = exeThreadData.pInterpreter->getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64;
		return (int)exitCode;
	}
}
