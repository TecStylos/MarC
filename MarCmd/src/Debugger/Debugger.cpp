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

		struct ModuleDisasmInfo
		{
			std::vector<uint64_t> instructionOffsets;
			std::vector<std::string> instructions;
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
				auto daii = MarC::Disassembler::disassemble((char*)mem->getBaseAddress() + nDisassembled);
				inf.instructions.push_back(MarC::DisAsmInsInfoToString(daii, exeInfo->symbols));
				inf.instructionOffsets.push_back(nDisassembled);
				nDisassembled += daii.rawData.size();
			}
		}

		struct ExeThreadData
		{
			std::shared_ptr<MarC::Interpreter> pInterpreter = nullptr;

			uint64_t exeCount = 0;
			std::mutex mtxExeCount;
			std::condition_variable conExeCount;

			std::atomic_bool stopExecution = false;
			std::atomic_bool threadClosed = false;
		} exeThreadData;
		exeThreadData.pInterpreter = std::make_shared<MarC::Interpreter>(exeInfo);

		std::thread exeThread(
			[](ExeThreadData* pEtd)
			{
				auto& etd = *pEtd;
				while (!etd.pInterpreter->lastError() && !etd.stopExecution)
				{
					std::unique_lock lock(etd.mtxExeCount);
					etd.conExeCount.wait(lock, [&]() { return etd.exeCount != 0 || etd.stopExecution; });

					while (etd.exeCount > 0 && !etd.pInterpreter->lastError() && !etd.stopExecution)
					{
						--etd.exeCount;
						lock.unlock();

						etd.pInterpreter->interpret(1);

						lock.lock();
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

		Console::subTextWndInsert(wndFull, DbgWndName_DisasmTitle, "Disassembly:", 1, 0);
		Console::subTextWndInsert(wndFull, DbgWndName_ConsoleTitle, "Console:", 1, 0);
		Console::subTextWndInsert(wndFull, DbgWndName_InputView, ">> ", 1, 0);
		Console::subTextWndInsert(wndFull, DbgWndName_MemoryTitle, "Memory:", 1, 0);
		Console::subTextWndInsert(wndFull, DbgWndName_CallstackTitle, "Callstack:", 1, 0);
		Console::subTextWndInsert(wndFull, DbgWndName_DisasmViewControl, "->", 0, 0);

		auto wndDisasm = wndFull->getSubWndByName<Console::TextWindow>(DbgWndName_DisasmViewCode);
		if (wndDisasm)
		{
			for (auto& line : modDisasmInfo[0].instructions)
				wndDisasm->append(line + "\n");
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
				case 's': // [S]top the execution.
				{
					std::lock_guard lock(exeThreadData.mtxExeCount);
					exeThreadData.exeCount = 0;
					exeThreadData.conExeCount.notify_one();
					refreshRequested = true;
					break;
				}
				case 'n': // Execute the [n]ext instruction
				{
					std::lock_guard lock(exeThreadData.mtxExeCount);
					++exeThreadData.exeCount;
					exeThreadData.conExeCount.notify_one();
					refreshRequested = true;
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
							auto wndDisasmTitle = wndFull->getSubWndByName<Console::TextWindow>(DbgWndName_DisasmTitle);
							auto wndDisasmViewControl = wndFull->getSubWndByName<Console::TextWindow>(DbgWndName_DisasmViewControl);
							auto wndDisasmViewCode = wndFull->getSubWndByName<Console::TextWindow>(DbgWndName_DisasmViewCode);
							wndDisasmTitle->replace("Disassembly: " + exeInfo->modules[modIndex]->moduleName, 1, 0);
							int64_t mid = wndDisasmViewCode->getHeight() / 2;
							wndDisasmViewControl->setScroll(-mid);
							wndDisasmViewCode->setScroll(-mid + line);
						}

						{
							auto wndMemoryView = wndFull->getSubWndByName<Console::TextWindow>(DbgWndName_MemoryView);
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

		return 0;
	}
}
