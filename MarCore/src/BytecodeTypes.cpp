#include "BytecodeTypes.h"

#include <map>

namespace MarC
{
	bool BC_OpCodeEx::ArgDerefs::operator[](uint64_t index) const
	{
		return get(index);
	}

	bool BC_OpCodeEx::ArgDerefs::get(uint64_t index) const
	{
		return (data >> index) & 1;
	}

	void BC_OpCodeEx::ArgDerefs::set(uint64_t index)
	{
		data |= (1 << index);
	}

	void BC_OpCodeEx::ArgDerefs::clear(uint64_t index)
	{
		data &= ~(1 << index);
	}

	BC_MemAddress::BC_MemAddress(BC_MemBase base, uint64_t addr)
		: base(base), addr(addr)
	{
	}

	BC_MemAddress::BC_MemAddress(BC_MemBase base, uint64_t page, uint64_t addr)
	{
		asCode.base = base;
		asCode.page = page;
		asCode.addr = addr;
	}
  bool BC_MemAddress::operator<(const BC_MemAddress& other) const
  {
    if (base != other.base)
      return base < other.base;
    return addr < other.addr;
  }
  
	BC_Datatype BC_FuncCallData::ArgTypes::get(uint8_t nthArg)
	{
		uint8_t shift = 4 * nthArg;
		uint32_t val = data >> shift;
		uint32_t mask = (uint32_t)15;
		val &= mask;
		return (BC_Datatype)val;
	}

	void BC_FuncCallData::ArgTypes::set(uint8_t nthArg, BC_Datatype dt)
	{
		uint8_t shift = 4 * nthArg;
		uint32_t val = (uint32_t)dt << shift;
		uint32_t clrMask = ~((uint32_t)15 << shift);
		data &= clrMask;
		data |= val;
	}

	BC_OpCode BC_OpCodeFromString(const std::string& ocStr)
	{
		static const std::map<std::string, BC_OpCode> ocMap = {
			{ "",        BC_OC_NONE },

			{ "mov",     BC_OC_MOVE },

			{ "add",     BC_OC_ADD },
			{ "sub",     BC_OC_SUBTRACT },
			{ "mul",     BC_OC_MULTIPLY },
			{ "div",     BC_OC_DIVIDE },

			{ "drf",     BC_OC_DEREFERENCE },

			{ "conv",    BC_OC_CONVERT },

			{ "push",    BC_OC_PUSH },
			{ "pop",     BC_OC_POP },
			{ "pushc",   BC_OC_PUSH_COPY },
			{ "popc",    BC_OC_POP_COPY },

			{ "pushf",   BC_OC_PUSH_FRAME },
			{ "popf",    BC_OC_POP_FRAME },

			{ "jmp",     BC_OC_JUMP },
			{ "jeq",     BC_OC_JUMP_EQUAL },
			{ "jne",     BC_OC_JUMP_NOT_EQUAL },
			{ "jlt",     BC_OC_JUMP_LESS_THAN },
			{ "jgt",     BC_OC_JUMP_GREATER_THAN },
			{ "jle",     BC_OC_JUMP_LESS_EQUAL },
			{ "jge",     BC_OC_JUMP_GREATER_EQUAL },

			{ "alloc",   BC_OC_ALLOCATE },
			{ "free",    BC_OC_FREE },

			{ "calx",    BC_OC_CALL_EXTERN },
			
			{ "call",    BC_OC_CALL },
			{ "return",  BC_OC_RETURN },

			{ "exit",    BC_OC_EXIT },
		};

		auto it = ocMap.find(ocStr);
		if (it == ocMap.end())
			return BC_OC_UNKNOWN;
		return it->second;
	}
	std::string BC_OpCodeToString(BC_OpCode oc)
	{
		static const std::map<BC_OpCode, std::string> ocMap = {
			{ BC_OC_NONE, "" },

			{ BC_OC_MOVE, "mov"},

			{ BC_OC_ADD, "add" },
			{ BC_OC_SUBTRACT, "sub" },
			{ BC_OC_MULTIPLY, "mul" },
			{ BC_OC_DIVIDE, "div" },

			{ BC_OC_DEREFERENCE, "drf" },

			{ BC_OC_CONVERT, "conv" },

			{ BC_OC_PUSH, "push" },
			{ BC_OC_POP, "pop" },
			{ BC_OC_PUSH_COPY, "pushc" },
			{ BC_OC_POP_COPY, "popc" },

			{ BC_OC_PUSH_FRAME, "pushf" },
			{ BC_OC_POP_FRAME, "popf" },

			{ BC_OC_JUMP, "jmp" },
			{ BC_OC_JUMP_EQUAL, "jeq" },
			{ BC_OC_JUMP_NOT_EQUAL, "jne" },
			{ BC_OC_JUMP_LESS_THAN, "jlt" },
			{ BC_OC_JUMP_GREATER_THAN, "jgt" },
			{ BC_OC_JUMP_LESS_EQUAL, "jle" },
			{ BC_OC_JUMP_GREATER_EQUAL, "jge" },

			{ BC_OC_ALLOCATE, "alloc" },
			{ BC_OC_FREE, "free" },
			
			{ BC_OC_CALL_EXTERN, "calx" },

			{ BC_OC_CALL, "call" },
			{ BC_OC_RETURN, "return" },

			{ BC_OC_EXIT, "exit" },
		};

		auto it = ocMap.find(oc);
		if (it == ocMap.end())
			return "<unknown>";
		return it->second;
	}

	BC_Datatype BC_DatatypeFromString(const std::string& dtStr)
	{
		static const std::map<std::string, BC_Datatype> dtMap = {
			{ "none", BC_DT_NONE },
			{ "i8",   BC_DT_I_8  },
			{ "i16",  BC_DT_I_16 },
			{ "i32",  BC_DT_I_32 },
			{ "i64",  BC_DT_I_64 },
			{ "u8",   BC_DT_U_8  },
			{ "u16",  BC_DT_U_16 },
			{ "u32",  BC_DT_U_32 },
			{ "u64",  BC_DT_U_64 },
			{ "f32",  BC_DT_F_32 },
			{ "f64",  BC_DT_F_64 },
			{ "addr", BC_DT_U_64 },
		};

		auto it = dtMap.find(dtStr);
		if (it == dtMap.end())
			return BC_DT_UNKNOWN;
		return it->second;
	}
	std::string BC_DatatypeToString(BC_Datatype dt)
	{
		static const std::map<BC_Datatype, std::string> dtMap = {
			{ BC_DT_NONE,    "none"  },
			{ BC_DT_I_8,     "i8"    },
			{ BC_DT_I_16,    "i16"   },
			{ BC_DT_I_32,    "i32"   },
			{ BC_DT_I_64,    "i64"   },
			{ BC_DT_U_8,     "u8"    },
			{ BC_DT_U_16,    "u16"   },
			{ BC_DT_U_32,    "u32"   },
			{ BC_DT_U_64,    "u64"   },
			{ BC_DT_F_32,    "f32"   },
			{ BC_DT_F_64,    "f64"   },
			{ BC_DT_U_64,    "addr"  },
		};

		auto it = dtMap.find(dt);
		if (it == dtMap.end())
			return "<unknown>";
		return it->second;
	}

	BC_MemRegister BC_RegisterFromString(const std::string& regStr)
	{
		static const std::map<std::string, BC_MemRegister> regMap = {
			{ "",        BC_MEM_REG_NONE           },
			{ "cp",      BC_MEM_REG_CODE_POINTER   },
			{ "sp",      BC_MEM_REG_STACK_POINTER  },
			{ "fp",      BC_MEM_REG_FRAME_POINTER  },
			{ "lc",      BC_MEM_REG_LOOP_COUNTER   },
			{ "ac",      BC_MEM_REG_ACCUMULATOR    },
			{ "td",      BC_MEM_REG_TEMPORARY_DATA },
			{ "ec",      BC_MEM_REG_EXIT_CODE      },
		};

		auto it = regMap.find(regStr);
		if (it == regMap.end())
			return BC_MEM_REG_UNKNOWN;
		return it->second;
	}
	std::string BC_RegisterToString(BC_MemRegister reg)
	{
		static const std::map<BC_MemRegister, std::string> regMap = {
			{ BC_MEM_REG_NONE,           ""         },
			{ BC_MEM_REG_CODE_POINTER,   "cp"       },
			{ BC_MEM_REG_STACK_POINTER,  "sp"       },
			{ BC_MEM_REG_FRAME_POINTER,  "fp"       },
			{ BC_MEM_REG_LOOP_COUNTER,   "lc"       },
			{ BC_MEM_REG_ACCUMULATOR,    "ac"       },
			{ BC_MEM_REG_TEMPORARY_DATA, "td"       },
			{ BC_MEM_REG_EXIT_CODE,      "ec"       },
		};

		auto it = regMap.find(reg);
		if (it == regMap.end())
			return "<unknown>";
		return it->second;
	}

	uint64_t BC_DatatypeSize(BC_Datatype dt)
	{
		switch (dt)
		{
		case BC_DT_I_8:
		case BC_DT_U_8:
			return 1;
		case BC_DT_I_16:
		case BC_DT_U_16:
			return 2;
		case BC_DT_I_32:
		case BC_DT_U_32:
		case BC_DT_F_32:
			return 4;
		case BC_DT_I_64:
		case BC_DT_U_64:
		case BC_DT_F_64:
			return 8;
		}

		return 0;
	}
}
