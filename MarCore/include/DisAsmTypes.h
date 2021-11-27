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
	};

	struct DisAsmInsInfo
	{
		BC_OpCodeEx ocx;
		std::vector<DisAsmArg> args;
		std::vector<char> rawData;
	};
}