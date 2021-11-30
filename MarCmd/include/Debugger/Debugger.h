#pragma once

#include <condition_variable>
#include <atomic>

#include <MarCore.h>

#include "MarCmdSettings.h"
#include "Debugger/DebugWindow.h"

namespace MarCmd
{
	class Debugger;

	typedef std::shared_ptr<class DisasmWindow> DisasmWindowRef;
	class DisasmWindow : public Console::SplitWindow
	{
	protected:
		DisasmWindow() = delete;
		DisasmWindow(const std::string& name, MarC::InterpreterRef interpreter, uint64_t modIndex);
		DisasmWindow(DisasmWindow&&) = delete;
		DisasmWindow(const DisasmWindow&) = delete;
	public:
		virtual void handleKeyPress(char key) override;
	public:
		std::vector<struct ModDisasmInfo> getInfo() const;
		void refresh();
	public:
		static DisasmWindowRef create(const std::string& name, MarC::InterpreterRef interpreter, uint64_t modIndex);
	private:
		struct ModDisasmInfo
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
		MarC::InterpreterRef m_interpreter;
		uint64_t m_modIndex;
		ModDisasmInfo m_modDisasmInfo;
	};

	class Debugger
	{
	public:
		static int run(const Settings& settings);
	private:
		Debugger(const Settings& settings);
	private:
		int run();
	private:
		void exeThreadFunc();
	private:
		Settings m_settings;
		MarC::ExecutableInfoRef m_exeInfo;
		MarC::InterpreterRef m_interpreter;
		Console::BaseWindowRef m_wndBase;
		DisasmWindowRef m_wndDisasm;
		std::vector<DisasmWindowRef> m_vecWndDisasm;
	private:
		uint64_t m_maxPrintSymLen = 0;
	private:
		struct ExeThreadData
		{
			uint64_t exeCount = 0;
			std::mutex mtxExeCount;
			std::condition_variable conExeCount;

			std::atomic_bool stopExecution = false;
			std::atomic_bool threadClosed = false;
			MarC::BC_Datatype regDatatypes[MarC::BC_MEM_REG_NUM_OF_REGS];
		} m_exeThreadData;
	};
}