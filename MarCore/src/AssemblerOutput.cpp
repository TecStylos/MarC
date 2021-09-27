#include "AssemblerOutput.h"

namespace MarC
{
	uint64_t BytecodeInfo::getErrLine() const
	{
		return nLinesParsed + 1;
	}

	BC_MemAddress::BC_MemAddress(BC_MemBase base, uint64_t addr)
		: base(base), address(addr)
	{
	}

	BC_OpCode BC_OpCodeFromString(const std::string& ocStr)
	{
		static constexpr struct
		{
			const char* asStr;
			BC_OpCode asOpCode;
		} codes[] = {
			{ "",       BC_OC_NONE },
			{ "mov",    BC_OC_MOVE },
			{ "add",    BC_OC_ADD },
			{ "sub",    BC_OC_SUBTRACT },
			{ "mul",    BC_OC_MULTIPLY },
			{ "div",    BC_OC_DIVIDE },
			{ "cpy",    BC_OC_COPY },
			{ "push",   BC_OC_PUSH },
			{ "pop",    BC_OC_POP },
			{ "pushf",  BC_OC_PUSH_FRAME },
			{ "popf",   BC_OC_POP_FRAME },
			{ "return", BC_OC_RETURN },
		};

		for (auto code : codes)
		{
			if (code.asStr == ocStr)
				return code.asOpCode;
		}

		return BC_OC_UNKNOWN;
	}

	BC_Datatype BC_DatatypeFromString(const std::string& dtStr)
	{
		static constexpr struct
		{
			const char* asStr;
			BC_Datatype asDatatype;
		} datatypes[] = {
			{ "",     BC_DT_NONE },
			{ "i8",   BC_DT_I_8 },
			{ "i16",  BC_DT_I_16 },
			{ "i32",  BC_DT_I_32 },
			{ "i64",  BC_DT_I_64 },
			{ "u8",   BC_DT_U_8  },
			{ "u16",  BC_DT_U_16 },
			{ "u32",  BC_DT_U_32 },
			{ "u64",  BC_DT_U_64 },
			{ "f32",  BC_DT_F_32 },
			{ "f64",  BC_DT_F_64 },
			{ "bool", BC_DT_BOOL },
		};

		for (auto dt : datatypes)
		{
			if (dt.asStr == dtStr)
				return dt.asDatatype;
		}

		return BC_DT_UNKNOWN;
	}

	BC_MemRegister BC_RegisterFromString(const std::string& regStr)
	{
		static constexpr struct
		{
			const char* asStr;
			BC_MemRegister asRegister;
		} registers[] = {
			{ "$",   BC_MEM_REG_NONE },
			{ "$sp", BC_MEM_REG_STACK_POINTER },
			{ "$fp", BC_MEM_REG_FRAME_POINTER },
			{ "$lc", BC_MEM_REG_LOOP_COUNTER },
			{ "$ac", BC_MEM_REG_ACCUMULATOR },
			{ "$cp", BC_MEM_REG_CODE_POINTER },
			{ "$ec", BC_MEM_REG_EXIT_CODE },
		};

		for (auto r : registers)
		{
			if (r.asStr == regStr)
				return r.asRegister;
		}

		return BC_MEM_REG_UNKNOWN;
	}

	uint64_t BC_DatatypeSize(BC_Datatype dt)
	{
		switch (dt)
		{
		case BC_DT_I_8:
		case BC_DT_U_8:
		case BC_DT_BOOL:
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