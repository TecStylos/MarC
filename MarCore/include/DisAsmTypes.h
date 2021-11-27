#pragma once

#include <vector>
#include "BytecodeTypes.h"
#include "AssemblerTypes.h"
#include "AsmInstructions.h"

namespace MarC
{
	struct DisAsmArg
	{
		bool getsDereferenced = false;
		TypeCell value;
		InsArgType argType;
	};

	struct DisAsmInsInfo
	{
		BC_OpCodeEx ocx;
		std::vector<DisAsmArg> args;
		std::vector<char> rawData;
	};

	std::string DisAsmInsInfoToString(const DisAsmInsInfo& daii, const std::set<Symbol>& symbols);
}