#pragma once

#include "Debugger/ConsoleWindow.h"

namespace MarCmd
{
	#define DbgWndName_Full "Full"
	#define DbgWndName_LeftHalf "Left Half"
	#define DbgWndName_ConsoleInputSplit "Console & Input"
	#define DbgWndName_ConsoleSplit "Console Split"
	#define DbgWndName_ConsoleTitle "Console Title"
	#define DbgWndName_ConsoleView "Console View"
	#define DbgWndName_InputView "Input View"
	#define DbgWndName_RightHalfWithSep "Right Half & Separator"
	#define DbgWndName_Separator "Separator"
	#define DbgWndName_RightHalf "Right Half"
	#define DbgWndName_MemorySplit "Memory Split"
	#define DbgWndName_MemoryTitle "Memory Title"
	#define DbgWndName_MemoryView "Memory View"
	#define DbgWndName_ModuleCallstackSplit "Module & Callstack"
	#define DbgWndName_ModuleSplit "Module Split"
	#define DbgWndName_ModuleTitle "Module Title"
	#define DbgWndName_ModuleView "Module View"
	#define DbgWndName_CallstackSplit "Callstack Split"
	#define DbgWndName_CallstackTitle "Callstack Title"
	#define DbgWndName_CallstackView "Callstack View"

	Console::BaseWindowRef createDebugWindow(uint64_t width, uint64_t height);
}