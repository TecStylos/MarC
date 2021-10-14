#pragma once

#include <cstdint>
#include <string>

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

		BC_OC_CONVERT,

		BC_OC_PUSH,
		BC_OC_POP,
		BC_OC_PUSH_COPY,
		BC_OC_POP_COPY,

		BC_OC_PUSH_FRAME,
		BC_OC_POP_FRAME,

		BC_OC_JUMP,

		BC_OC_EXIT,

		BC_OC_NUM_OF_OP_CODES,
	};

	enum BC_MemBase
	{
		BC_MEM_BASE_NONE = 0,
		BC_MEM_BASE_STATIC_STACK,
		BC_MEM_BASE_DYNAMIC_STACK,
		BC_MEM_BASE_DYN_FRAME_ADD,
		BC_MEM_BASE_DYN_FRAME_SUB,
		BC_MEM_BASE_CODE_MEMORY,
		BC_MEM_BASE_REGISTER,
		BC_MEM_BASE_EXTERN,
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
		BC_MEM_REG_NUM_OF_REGS,
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
		union
		{
			struct
			{
				uint64_t base : 3;
				uint64_t addr : 61;
			};
			struct
			{
				uint64_t base : 3;
				uint64_t page : 5;
				uint64_t addr : 56;
			} asCode;
		};
	public:
		BC_MemAddress() = default;
		BC_MemAddress(BC_MemBase base, uint64_t addr);
		BC_MemAddress(BC_MemBase base, uint64_t page, uint64_t addr);
	};

	union BC_MemCell
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
		BC_MemAddress as_ADDR;
	public:
		BC_MemCell() = default;
		BC_MemCell(int64_t val) : as_I_64(val) {}
		BC_MemCell(uint64_t val) : as_U_64(val) {}
		BC_MemCell(float val) : as_F_32(val) {}
		BC_MemCell(double val) : as_F_64(val) {}
		BC_MemCell(bool val) : as_BOOL(val) {}
		BC_MemCell(BC_MemAddress val) : as_ADDR(val) {}
	};

	#pragma pack(pop)

	inline constexpr uint64_t BC_MemRegisterID(BC_MemRegister memReg)
	{
		return memReg - 2;
	}

	BC_OpCode BC_OpCodeFromString(const std::string& ocStr);

	BC_Datatype BC_DatatypeFromString(const std::string& dtStr);

	BC_MemRegister BC_RegisterFromString(const std::string& regStr);

	uint64_t BC_DatatypeSize(BC_Datatype dt);
}