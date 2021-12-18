#pragma once

#include <condition_variable>
#include <atomic>
#include <mutex>

#include <MarCore.h>

#include "MarCmdSettings.h"
#include "Debugger/DebugWindow.h"
#include "types/BytecodeTypes.h"

namespace MarCmd
{
	struct SharedDebugData
	{
		MarC::ExecutableInfoRef exeInfo;
		MarC::InterpreterRef interpreter;
		bool refreshRequested = false;
		Console::BaseWindowRef wndBase;

		uint64_t exeCount = 0;
		std::mutex mtxExeCount;
		std::condition_variable conExeCount;

		std::atomic_bool stopExecution = false;
		std::atomic_bool threadClosed = false;
		MarC::BC_Datatype regDatatypes[MarC::_BC_MEM_REG_NUM] = { MarC::BC_DT_UNKNOWN };
	};
	typedef std::shared_ptr<SharedDebugData> SharedDebugDataRef;

	class Debugger;

	typedef std::shared_ptr<class DisasmWindow> DisasmWindowRef;
	class DisasmWindow : public Console::SplitWindow
	{
	public:
		struct ModDisasmInfo
		{
			std::vector<int64_t> instructionOffsets;
			struct InsInfo
			{
				std::string str;
				MarC::DisAsmInsInfo data;
			};
			std::vector<InsInfo> ins;
			std::mutex mtxBreakpoints;
			std::set<MarC::BC_MemAddress> breakpoints;
		public:
			MarC::BC_MemAddress lineToAddr(int64_t line) const { return MarC::BC_MemAddress(MarC::BC_MEM_BASE_CODE_MEMORY, instructionOffsets[line]); }
			int64_t addrToLine(MarC::BC_MemAddress addr) const { return MarC::searchBinary(addr.addr, instructionOffsets); }
		};
	protected:
		DisasmWindow() = delete;
		DisasmWindow(const std::string& name, SharedDebugDataRef sdd, uint64_t modIndex);
		DisasmWindow(DisasmWindow&&) = delete;
		DisasmWindow(const DisasmWindow&) = delete;
	public:
		virtual ~DisasmWindow() = default;
	public:
		virtual void handleKeyPress(char key) override;
	public:
		std::vector<struct ModDisasmInfo> getInfo() const;
		void refresh();
		bool hasBreakpoint(MarC::BC_MemAddress breakpoint);
		bool toggleBreakpoint(MarC::BC_MemAddress brekpoint);
		const ModDisasmInfo& getDisasmInfo() const;
	public:
		static DisasmWindowRef create(const std::string& name, SharedDebugDataRef sdd, uint64_t modIndex);
	private:
		SharedDebugDataRef m_sdd;
		uint64_t m_modIndex;
		int64_t m_scrollOffset = 0;
		uint64_t m_nInsExecuted;
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
		DisasmWindowRef m_wndDisasm;
		std::vector<DisasmWindowRef> m_vecWndDisasm;
	private:
		uint64_t m_maxPrintSymLen = 0;
	private:
		SharedDebugDataRef m_sharedDebugData;
	};
}