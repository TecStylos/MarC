#pragma once

#include <cstdint>
#include <string>

#include "Serializer.h"

namespace MarC
{
	enum BC_Datatype : uint8_t
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
		BC_DT_DATATYPE,
		BC_DT_ADDR,
	};
	MARC_SERIALIZER_ENABLE_FIXED(BC_Datatype);
	enum BC_OpCode : uint8_t
	{
		BC_OC_NONE = 0,
		BC_OC_UNKNOWN,

		BC_OC_MOVE,

		BC_OC_ADD,
		BC_OC_SUBTRACT,
		BC_OC_MULTIPLY,
		BC_OC_DIVIDE,
		BC_OC_INCREMENT,
		BC_OC_DECREMENT,

		BC_OC_DEREFERENCE,

		BC_OC_CONVERT,

		BC_OC_PUSH,
		BC_OC_POP,
		BC_OC_PUSH_N_BYTES,
		BC_OC_POP_N_BYTES,
		BC_OC_PUSH_COPY,
		BC_OC_POP_COPY,

		BC_OC_PUSH_FRAME,
		BC_OC_POP_FRAME,

		BC_OC_JUMP,
		BC_OC_JUMP_EQUAL,
		BC_OC_JUMP_NOT_EQUAL,
		BC_OC_JUMP_LESS_THAN,
		BC_OC_JUMP_GREATER_THAN,
		BC_OC_JUMP_LESS_EQUAL,
		BC_OC_JUMP_GREATER_EQUAL,

		BC_OC_ALLOCATE,
		BC_OC_FREE,
		
		BC_OC_CALL_EXTERN,

		BC_OC_CALL,
		BC_OC_RETURN,

		BC_OC_EXIT,

		BC_OC_NUM_OF_OP_CODES,
	};

	enum BC_MemBase : uint8_t
	{
		BC_MEM_BASE_NONE = 0,
		BC_MEM_BASE_STATIC_STACK,
		BC_MEM_BASE_DYNAMIC_STACK,
		BC_MEM_BASE_DYNAMIC_FRAME,
		BC_MEM_BASE_CODE_MEMORY,
		BC_MEM_BASE_REGISTER,
		BC_MEM_BASE_EXTERN,
		_BC_MEM_BASE_NUM,
	};
	enum BC_MemRegister : uint8_t
	{
		BC_MEM_REG_NONE = 0 * sizeof(uint64_t),
		BC_MEM_REG_UNKNOWN = 1 * sizeof(uint64_t),
		BC_MEM_REG_CODE_POINTER = 2 * sizeof(uint64_t),
		BC_MEM_REG_STACK_POINTER = 3 * sizeof(uint64_t),
		BC_MEM_REG_FRAME_POINTER = 4 * sizeof(uint64_t),
		BC_MEM_REG_LOOP_COUNTER = 5 * sizeof(uint64_t),
		BC_MEM_REG_ACCUMULATOR = 6 * sizeof(uint64_t),
		BC_MEM_REG_TEMPORARY_DATA = 7 * sizeof(uint64_t),
		BC_MEM_REG_EXIT_CODE = 8 * sizeof(uint64_t),
		_BC_MEM_REG_NUM = 9,
	};

	#pragma pack(push, 1)

	struct BC_OpCodeEx
	{
		BC_OpCode opCode = BC_OC_NONE;
		BC_Datatype datatype = BC_DT_NONE;
		struct ArgDerefs
		{
			uint8_t data = 0;
			bool operator[](uint8_t index) const { return get(index); }
			bool get(uint8_t index) const { return (data >> index) & 1; }
			void set(uint8_t index) { data |= (1 << index); }
			void clear(uint8_t index) { data &= ~(1 << index); }
		} derefArg;
	};

	struct BC_MemAddress
	{
		union
		{
			struct
			{
				int64_t addr : 56;
				int64_t base : 8;
			};
			uint64_t _raw;
		};
	public:
		BC_MemAddress() = default;
		BC_MemAddress(BC_MemBase base, int64_t addr) : addr(addr), base(base) {}
		bool operator<(const BC_MemAddress& other) const { return _raw < other._raw; }
		bool operator>(const BC_MemAddress& other) const { return _raw > other._raw; }
		bool operator<=(const BC_MemAddress& other) const { return _raw <= other._raw; }
		bool operator>=(const BC_MemAddress& other) const { return _raw >= other._raw; }
		bool operator==(const BC_MemAddress& other) const { return _raw == other._raw; }
		bool operator!=(const BC_MemAddress& other) const { return _raw != other._raw; }
		BC_MemAddress& operator+=(const BC_MemAddress& other) { _raw += other._raw; return *this; }
		BC_MemAddress& operator-=(const BC_MemAddress& other) { _raw -= other._raw; return *this; }
		BC_MemAddress& operator*=(const BC_MemAddress& other) { _raw *= other._raw; return *this; }
		BC_MemAddress& operator/=(const BC_MemAddress& other) { _raw /= other._raw; return *this; }

		BC_MemAddress& operator=(const BC_MemAddress& other) { _raw = other._raw; return *this; }
	};

	struct BC_MemCell
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
			BC_MemAddress as_ADDR;
			BC_Datatype as_Datatype;
		};
	public:
		BC_MemCell() { as_U_64 = 0; };
		BC_MemCell(BC_MemAddress val) : as_ADDR(val) {}
		BC_MemCell& operator=(const BC_MemCell& other) { as_U_64 = other.as_U_64; return *this; }
	};
	MARC_SERIALIZER_ENABLE_FIXED(BC_MemCell);

	struct BC_FuncCallData
	{
		uint8_t nArgs = 0;
		struct ArgTypes
		{
			uint32_t data = 0;
			BC_Datatype get(uint8_t nthArg) const { return (BC_Datatype)((data >> (4 * nthArg)) & (uint32_t)15); }
			void set(uint8_t nthArg, BC_Datatype dt) { data &= ~((uint32_t)15 << (4 * nthArg)); data |= ((uint32_t)dt << (4 * nthArg)); }
		} argType;
	};

	#pragma pack(pop)

	BC_OpCode BC_OpCodeFromString(const std::string& ocStr);
	std::string BC_OpCodeToString(BC_OpCode oc);

	BC_Datatype BC_DatatypeFromString(const std::string& dtStr);
	std::string BC_DatatypeToString(BC_Datatype dt);

	BC_MemRegister BC_RegisterFromString(const std::string& regStr);
	std::string BC_RegisterToString(BC_MemRegister reg);

	std::string BC_MemCellToString(BC_MemCell mc, BC_Datatype dt);

	std::string BC_MemAddressToString(BC_MemAddress addr);

	inline uint64_t BC_DatatypeSize(BC_Datatype dt)
	{
		static constexpr uint64_t sizeTable[] = {
			0, 0, // NONE, UNKNOWN
			1, 2, 4, 8, // I8, I16, I32, I64
			1, 2, 4, 8, // U8, U16, U32, U64
			4, 8, // F32, F64
			1, 8 // DATATYPE, ADDR
		};
		return sizeTable[dt];
	}
}
