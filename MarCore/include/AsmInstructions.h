#pragma once

#include "AssemblerTypes.h"

namespace MarC
{
	enum class InsArgType
	{
		None = 0,
		Address,
		Value,
		Datatype,
		TypedValue,
	};

	struct InsArgument
	{
		InsArgType type;
		BC_Datatype datatype;
		uint64_t index;
	public:
		InsArgument(InsArgType type, BC_Datatype datatype = BC_DT_NONE, uint64_t index = -1)
			: type(type), datatype(datatype), index(index)
		{}
	};

	enum class InsDt
	{
		None,
		Optional,
		Required,
	};

	struct InstructionLayout
	{
		BC_OpCode opCode;
		InsDt insDt;
		std::vector<InsArgument> args;
		bool needsCustomImplementation;
	public:
		InstructionLayout(BC_OpCode opCode, InsDt insDt, std::initializer_list<InsArgument> args, bool needsCustomImplementation = false)
			: opCode(opCode), insDt(insDt), args(args), needsCustomImplementation(needsCustomImplementation)
		{
			for (uint64_t i = 0; i < this->args.size(); ++i)
			{
				this->args[i].index = i;
			}
		}
	};

	static const InstructionLayout InstructionSet[] = {
		{ BC_OC_MOVE, InsDt::Required, { { InsArgType::Address }, { InsArgType::Value } } },
		{ BC_OC_ADD, InsDt::Required, { { InsArgType::Address }, { InsArgType::Value } } },
		{ BC_OC_SUBTRACT, InsDt::Required, { { InsArgType::Address }, { InsArgType::Value } } },
		{ BC_OC_MULTIPLY, InsDt::Required, { { InsArgType::Address }, { InsArgType::Value } } },
		{ BC_OC_DIVIDE, InsDt::Required, { { InsArgType::Address }, { InsArgType::Value } } },
		{ BC_OC_INCREMENT, InsDt::Required, { { InsArgType::Value } } },
		{ BC_OC_DECREMENT, InsDt::Required, { { InsArgType::Value } } },

		{ BC_OC_DEREFERENCE, InsDt::Required, { { InsArgType::Address }, { InsArgType::Address } } },

		{ BC_OC_CONVERT, InsDt::Required, { { InsArgType::Address }, { InsArgType::Datatype } } },

		{ BC_OC_PUSH, InsDt::Required, {} },
		{ BC_OC_POP, InsDt::Required, {} },
		{ BC_OC_PUSH_N_BYTES, InsDt::None, { { InsArgType::TypedValue, BC_DT_U_64 } } },
		{ BC_OC_POP_N_BYTES, InsDt::None, { { InsArgType::TypedValue, BC_DT_U_64 } } },
		{ BC_OC_PUSH_COPY, InsDt::Required, { { InsArgType::Value } } },
		{ BC_OC_POP_COPY, InsDt::Required, { { InsArgType::Address } } },

		{ BC_OC_PUSH_FRAME, InsDt::None, {} },
		{ BC_OC_POP_FRAME, InsDt::None, {} },

		{ BC_OC_JUMP, InsDt::None, { { InsArgType::Address } } },
		{ BC_OC_JUMP_EQUAL, InsDt::Required, { { InsArgType::Address }, { InsArgType::Value }, { InsArgType::Value } } },
		{ BC_OC_JUMP_NOT_EQUAL, InsDt::Required, { { InsArgType::Address }, { InsArgType::Value }, { InsArgType::Value } } },
		{ BC_OC_JUMP_LESS_THAN, InsDt::Required, { { InsArgType::Address }, { InsArgType::Value }, { InsArgType::Value } } },
		{ BC_OC_JUMP_GREATER_THAN, InsDt::Required, { { InsArgType::Address }, { InsArgType::Value }, { InsArgType::Value } } },
		{ BC_OC_JUMP_LESS_EQUAL, InsDt::Required, { { InsArgType::Address }, { InsArgType::Value }, { InsArgType::Value } } },
		{ BC_OC_JUMP_GREATER_EQUAL, InsDt::Required, { { InsArgType::Address }, { InsArgType::Value }, { InsArgType::Value } } },

		{ BC_OC_ALLOCATE, InsDt::None, { { InsArgType::Address }, { InsArgType::TypedValue, BC_DT_U_64 } } },
		{ BC_OC_FREE, InsDt::None, { { InsArgType::Address } } },

		{ BC_OC_CALL_EXTERN, InsDt::Optional, {}, true },

		{ BC_OC_CALL, InsDt::Optional, {}, true },
		{ BC_OC_RETURN, InsDt::None, {} },

		{ BC_OC_EXIT, InsDt::None, {} },
	};

	const InstructionLayout& InstructionLayoutFromOpCode(BC_OpCode oc);
}
