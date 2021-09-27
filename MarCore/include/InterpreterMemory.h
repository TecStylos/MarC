#pragma once

#include "AssemblerOutput.h"
#include "Memory.h"

namespace MarC
{
	struct InterpreterMemory
	{
		BC_MemCell registers[BC_MemRegisterID(BC_MEM_REG_NUM_OF_REGS)];
		MemoryRef staticStack;
		MemoryRef dynamicStack;
		MemoryRef codeMemory;
		// dynamic memory manager
	};
}