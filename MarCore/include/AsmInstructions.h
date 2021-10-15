#pragma once

#include "CompilerTypes.h"

namespace MarC
{
	enum class InsArgType
	{
		None = 0,
		Address,
		Value,
		Datatype,
	};

	struct InsArgument
	{
		InsArgType type;
		uint64_t index = -1;
	public:
		InsArgument(InsArgType type)
			: type(type)
		{}
	};

	struct InstructionLayout
	{
		BC_OpCode opCode;
		bool requiresDatatype;
		std::vector<InsArgument> args;
	public:
		InstructionLayout(BC_OpCode opCode, bool requiresDatatype, std::initializer_list<InsArgument> args)
			: opCode(opCode), requiresDatatype(requiresDatatype), args(args)
		{
			for (uint64_t i = 0; i < this->args.size(); ++i)
			{
				this->args[i].index = i;
			}
		}
	};

	static const InstructionLayout InstructionSet[] = {
		{ BC_OC_MOVE, true, { { InsArgType::Address }, { InsArgType::Value } } },
		{ BC_OC_ADD, true, { { InsArgType::Address }, { InsArgType::Value } } },
		{ BC_OC_SUBTRACT, true, { { InsArgType::Address }, { InsArgType::Value } } },
		{ BC_OC_MULTIPLY, true, { { InsArgType::Address }, { InsArgType::Value } } },
		{ BC_OC_DIVIDE, true, { { InsArgType::Address }, { InsArgType::Value } } },

		{ BC_OC_CONVERT, true, { { InsArgType::Address }, { InsArgType::Datatype } } },

		{ BC_OC_PUSH, true, {} },
		{ BC_OC_POP, true, {} },
		{ BC_OC_PUSH_COPY, true, { { InsArgType::Value } } },
		{ BC_OC_POP_COPY, true, { { InsArgType::Address } } },

		{ BC_OC_PUSH_FRAME, false, {} },
		{ BC_OC_POP_FRAME, false, {} },

		{ BC_OC_JUMP, false, { { InsArgType::Address } } },
		{ BC_OC_JUMP_EQUAL, true, { { InsArgType::Address }, { InsArgType::Value }, { InsArgType::Value } } },
		{ BC_OC_JUMP_NOT_EQUAL, true, { { InsArgType::Address }, { InsArgType::Value }, { InsArgType::Value } } },

		{ BC_OC_EXIT, false, {} }
	};

	const InstructionLayout& InstructionLayoutFromOpCode(BC_OpCode oc);
}