#pragma once

#include <map>

#include "Memory.h"
#include "BytecodeTypes.h"

namespace MarC
{
	struct InterpreterMemory
	{
		BC_MemCell registers[BC_MEM_REG_NUM_OF_REGS];
		Memory dynamicStack;
		void* dynamicStackBase = nullptr;
		void* codeMemBase = nullptr;
		uint64_t nextDynAddr = 0;
		std::map<BC_MemAddress, void*> dynMemMap;
	};
}
