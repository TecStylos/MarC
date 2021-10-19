#pragma once

#include "Memory.h"
#include "BytecodeTypes.h"

namespace MarC
{
	struct InterpreterMemory
	{
		BC_MemCell registers[BC_MEM_REG_NUM_OF_REGS];
		MemoryRef dynamicStack;
		// dynamic memory manager
	};
}