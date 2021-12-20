#pragma once

#include <vector>
#include "BytecodeTypes.h"
#include "AssemblerTypes.h"
#include "AsmInstructions.h"

namespace MarC
{
	struct DisAsmArg
	{
		DerefCount derefCount = 0;
		TypeCell value;
		InsArgType argType;
	};

	struct DisAsmInsInfo
	{
		BC_OpCodeEx ocx;
		std::vector<DisAsmArg> args;
		std::vector<char> rawData;
	};

	std::set<Symbol>::const_iterator getSymbolForAddress(BC_MemAddress addr, const std::set<Symbol>& symbols, const std::string& scopeName = "");

	std::string DisAsmInsInfoToString(const DisAsmInsInfo& daii, const std::set<Symbol>& symbols);
}