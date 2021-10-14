#pragma once

#include "CompilerTypes.h"

namespace MarC
{
	enum class InsArgType
	{
		None = 0,
		Address,
		Value
	};

	struct InsArgument
	{
		InsArgType type;
	public:
		InsArgument(InsArgType type)
			: type(type)
		{}
	};

	struct Instruction
	{
		BC_OpCode opCode;
		bool requiresDatatype;
		uint64_t nArgs;
		std::vector<InsArgument> args;
	public:
		Instruction(BC_OpCode opCode, bool requiresDatatype, std::initializer_list<InsArgument> args)
			: opCode(opCode), requiresDatatype(requiresDatatype), nArgs(nArgs), args(args)
		{}
	};

	static const Instruction instructionSet[] = {
		{
			BC_OC_MOVE,
			true,
			{ { InsArgType::Address }, { InsArgType::Value } }
		},
		{
			BC_OC_ADD,
			true,
			{ { InsArgType::Address }, { InsArgType::Value } }
		},
		{
			BC_OC_SUBTRACT,
			true,
			{ { InsArgType::Address }, { InsArgType::Value } }
		},
	};
}