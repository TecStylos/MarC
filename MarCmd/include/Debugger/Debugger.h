#pragma once

#include <condition_variable>
#include <atomic>
#include <mutex>

#include <MarCore.h>

#include "MarCmdSettings.h"
#include "Debugger/DebugWindow.h"

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
		MarC::BC_Datatype regDatatypes[MarC::BC_MEM_REG_NUM_OF_REGS] = { MarC::BC_DT_UNKNOWN };
	};
	typedef std::shared_ptr<SharedDebugData> SharedDebugDataRef;

	class Debugger;

	typedef std::shared_ptr<class DisasmWindow> DisasmWindowRef;
	class DisasmWindow : public Console::SplitWindow
	{
	protected:
		DisasmWindow() = delete;
		DisasmWindow(const std::string& name, SharedDebugDataRef sdd, uint64_t modIndex);
		DisasmWindow(DisasmWindow&&) = delete;
		DisasmWindow(const DisasmWindow&) = delete;
	public:
		virtual void handleKeyPress(char key) override;
	public:
		std::vector<struct ModDisasmInfo> getInfo() const;
		uint64_t getModIndex() const;
		void refresh();
		bool hasBreakpoint(MarC::BC_MemAddress breakpoint);
		bool toggleBreakpoint(MarC::BC_MemAddress brekpoint);
		int64_t addrToLine(MarC::BC_MemAddress addr) const;
	public:
		static DisasmWindowRef create(const std::string& name, SharedDebugDataRef sdd, uint64_t modIndex);
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
			std::mutex mtxBreakpoints;
			std::set<MarC::BC_MemAddress> breakpoints;
		};
		SharedDebugDataRef m_sdd;
		uint64_t m_modIndex;
		int64_t m_scrollOffset = 0;
		uint64_t m_nInsExecuted;
		ModDisasmInfo m_modDisasmInfo;
	};

	typedef std::shared_ptr<class ModuleBrowserWindow> ModuleBrowserWindowRef;
	class ModuleBrowserWindow : public Console::SplitWindow
	{
	protected:
		ModuleBrowserWindow() = delete;
		ModuleBrowserWindow(const std::string& name, SharedDebugDataRef sdd);
		ModuleBrowserWindow(ModuleBrowserWindow&&) = delete;
		ModuleBrowserWindow(const ModuleBrowserWindow&) = delete;
	public:
		virtual void handleKeyPress(char key) override;
	public:
		void refresh();
	public:
		static ModuleBrowserWindowRef create(const std::string& name, SharedDebugDataRef sdd);
	private:
		SharedDebugDataRef m_sdd;
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