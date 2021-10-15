#include "AsmInstructions.h"

namespace MarC
{
	const InstructionLayout& InstructionLayoutFromOpCode(BC_OpCode oc)
	{
		for (auto& il : InstructionSet)
		{
			if (oc == il.opCode)
				return il;
		}
	}
}