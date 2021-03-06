#pragma once

#include <map>

#include "Memory.h"
#include "types/BytecodeTypes.h"

namespace MarC
{
	struct InterpreterMemory
	{
		BC_MemCell registers[_BC_MEM_REG_NUM];
		Memory dynamicStack;
		void* baseTable[_BC_MEM_BASE_NUM] = { nullptr };
		uint64_t codeMemSize = 0;
		int64_t nextDynAddr = 0;
		std::map<BC_MemAddress, void*> dynMemMap;
	};
}
