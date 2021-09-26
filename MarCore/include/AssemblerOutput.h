#pragma once

#include <map>
#include <vector>
#include <stdint.h>
#include <string>

#include "Memory.h"

namespace MarC
{
	enum BC_Datatype : uint16_t
	{
		BC_DT_NONE = 0,
		BC_DT_UNKNOWN,

		BC_DT_I_8,
		BC_DT_I_16,
		BC_DT_I_32,
		BC_DT_I_64,
		BC_DT_U_8,
		BC_DT_U_16,
		BC_DT_U_32,
		BC_DT_U_64,
		BC_DT_F_32,
		BC_DT_F_64,
		BC_DT_BOOL
	};
	enum BC_OpCode : uint16_t
	{
		BC_OC_NONE = 0,
		BC_OC_UNKNOWN,

		BC_OC_MOVE,

		BC_OC_ADD,
		BC_OC_SUBTRACT,
		BC_OC_MULTIPLY,
		BC_OC_DIVIDE,

		BC_OC_COPY,

		BC_OC_PUSH,
		BC_OC_POP,

		BC_OC_PUSH_FRAME,
		BC_OC_POP_FRAME,

		BC_OC_CALL,
		BC_OC_RETURN,
	};

	enum BC_MemBase
	{
		BC_MEM_BASE_NONE = 0,
		BC_MEM_BASE_UNKNOWN,
		BC_MEM_BASE_STATIC_STACK,
		BC_MEM_BASE_DYNAMIC_STACK,
		BC_MEM_BASE_DYNAMIC_MEM,
		BC_MEM_BASE_CODE_MEM,
		BC_MEM_BASE_REGISTER,
	};
	enum BC_MemOffsetDirection
	{
		BC_MEM_OFFDIR_POSITIVE = 0,
		BC_MEM_OFFDIR_NEGATIVE = 1,
	};
	enum BC_MemRegister
	{
		BC_MEM_REG_NONE,
		BC_MEM_REG_UNKNOWN,
		BC_MEM_REG_STACK_POINTER,
		BC_MEM_REG_FRAME_POINTER,
		BC_MEM_REG_LOOP_COUNTER,
		BC_MEM_REG_ACCUMULATOR,
		BC_MEM_REG_CODE_POINTER,
		BC_MEM_REG_EXIT_CODE,
	};

	#pragma pack(push, 1)

	struct BC_OpCodeEx
	{
		uint16_t opCode : 10;
		uint16_t datatype : 4;
		uint16_t derefArg0 : 1;
		uint16_t derefArg1 : 1;
	};

	struct BC_MemAddress
	{
		uint64_t base : 3;
		uint64_t offsetDir : 1;
		uint64_t address : 60;
	};

	#pragma pack(pop)

	struct BytecodeInfo
	{
		std::map<std::string, BC_MemAddress> labels;
		std::vector<std::pair<std::string, uint64_t>> unresolvedRefs;
		std::vector<std::pair<std::string, uint64_t>> unresolvedRefsStaged;
		std::vector<uint64_t> resolvedRefAddresses;
		Memory staticStack;
		Memory codeMemory;
		uint64_t nLinesParsed = 0;
	public:
		uint64_t getErrLine() const;
	};

	struct BC_TypeCell
	{
		union
		{
			int8_t as_I_8;
			int16_t as_I_16;
			int32_t as_I_32;
			int64_t as_I_64;
			uint8_t as_U_8;
			uint16_t as_U_16;
			uint32_t as_U_32;
			uint64_t as_U_64;
			float as_F_32;
			double as_F_64;
			bool as_BOOL;
		} data;
		BC_Datatype datatype;
	};

	BC_OpCode BC_OpCodeFromString(const std::string& ocStr);

	BC_Datatype BC_DatatypeFromString(const std::string& dtStr);

	BC_MemRegister BC_RegisterFromString(const std::string& regStr);

	uint64_t BC_DatatypeSize(BC_Datatype dt);
}