#include "AsmInstructions.h"

namespace MarC
{
	const InstructionLayout& InstructionLayoutFromOpCode(BC_OpCode oc)
	{
	  static const InstructionLayout emptyLayout(BC_OC_NONE, InsDt::Optional, {});
		for (auto& il : InstructionSet)
		{
			if (oc == il.opCode)
				return il;
		}
		return emptyLayout;
	}
}
