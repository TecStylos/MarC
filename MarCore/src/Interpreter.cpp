#include "Interpreter.h"

namespace MarC
{
	Interpreter::Interpreter(MemoryRef staticStack, MemoryRef codeMemory, uint64_t dynStackSize)
	{
		initMemory(staticStack, codeMemory, dynStackSize);
	}

	bool Interpreter::interpret(uint64_t nInstructions)
	{
		if (!nInstructions)
			return true;

		while (nInstructions--)
			if (!execNext())
				break;

		return nInstructions == RunTillEOC;
	}

	void* Interpreter::hostAddress(BC_MemAddress clientAddr, bool deref)
	{
		void* hostAddr = nullptr;

		switch (clientAddr.base)
		{
		case BC_MEM_BASE_NONE:
			break;
		case BC_MEM_BASE_STATIC_STACK:
			hostAddr = (char*)m_mem.staticStack->getBaseAddress() + clientAddr.address;
			break;
		case BC_MEM_BASE_DYNAMIC_STACK:
			hostAddr = (char*)m_mem.dynamicStack->getBaseAddress() + clientAddr.address;
			break;
		case BC_MEM_BASE_DYN_FRAME_ADD:
			hostAddr = (char*)hostAddress(getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR) + clientAddr.address;
			break;
		case BC_MEM_BASE_DYN_FRAME_SUB:
			hostAddr = (char*)hostAddress(getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR) - clientAddr.address;
			break;
		case BC_MEM_BASE_CODE_MEMORY:
			hostAddr = (char*)m_mem.codeMemory->getBaseAddress() + clientAddr.address;
			break;
		case BC_MEM_BASE_REGISTER:
			hostAddr = &m_mem.registers[clientAddr.address];
			break;
		case BC_MEM_BASE_EXTERN:
			hostAddr = (char*)0 + clientAddr.address;
			break;
		}

		if (deref && hostAddr) // TODO: Error handling if hostAddr == nullptr
			hostAddr = hostAddress(*(BC_MemAddress*)hostAddr);
		return hostAddr;

	}

	BC_MemAddress Interpreter::clientAddress(void* hostAddr, BC_MemBase base)
	{
		return BC_MemAddress(
			base,
			(uint64_t)hostAddr - (uint64_t)hostAddress(
				BC_MemAddress(base, 0),
				false
			)
		);
	}

	BC_MemCell& Interpreter::getRegister(BC_MemRegister reg)
	{
		return m_mem.registers[BC_MemRegisterID(reg)];
	}

	void Interpreter::initMemory(MemoryRef staticStack, MemoryRef codeMemory, uint64_t dynStackSize)
	{
		if (staticStack)
			m_mem.staticStack = staticStack;
		if (codeMemory)
			m_mem.codeMemory = codeMemory;

		m_mem.dynamicStack = std::make_shared<Memory>(dynStackSize);

		getRegister(BC_MEM_REG_STACK_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_DYNAMIC_STACK, 0);
		getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_DYNAMIC_STACK, 0);
		getRegister(BC_MEM_REG_LOOP_COUNTER).as_U_64 = 0;
		getRegister(BC_MEM_REG_ACCUMULATOR).as_U_64 = 0;
		getRegister(BC_MEM_REG_CODE_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_CODE_MEMORY, 0);
		getRegister(BC_MEM_REG_EXIT_CODE).as_U_64 = 0;
	}

	BC_MemCell& Interpreter::readMemCellAndMove(BC_Datatype dt, bool deref)
	{
		return deref
			? *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), false)
			: readCodeAndMove<BC_MemCell>(BC_DatatypeSize(dt));
	}

	bool Interpreter::execNext()
	{
		static_assert(BC_OC_NUM_OF_OP_CODES == 18);

		auto ocx = readCodeAndMove<BC_OpCodeEx>();

		switch (ocx.opCode)
		{
		case BC_OC_NONE:
		case BC_OC_UNKNOWN:
			return false;
		case BC_OC_MOVE:
			return exec_insMove(ocx);
		case BC_OC_ADD:
			return exec_insAdd(ocx);
		case BC_OC_SUBTRACT:
			return exec_insSubtract(ocx);
		case BC_OC_MULTIPLY:
			return exec_insMultiply(ocx);
		case BC_OC_DIVIDE:
			return exec_insDivide(ocx);

		case BC_OC_CONVERT:
			return exec_insConvert(ocx);

		case BC_OC_COPY:
			return false;

		case BC_OC_PUSH:
			return exec_insPush(ocx);
		case BC_OC_POP:
			return exec_insPop(ocx);
		case BC_OC_PUSHC:
			return exec_insPushCopy(ocx);
		case BC_OC_POPC:
			return exec_insPopCopy(ocx);

		case BC_OC_PUSH_FRAME:
			return false;
		case BC_OC_POP_FRAME:
			return false;

		case BC_OC_CALL:
			return false;
		case BC_OC_RETURN:
			return false;

		case BC_OC_EXIT:
			return false;
		}

		return false;
	}

	bool Interpreter::exec_insMove(BC_OpCodeEx ocx)
	{
		void* dest = hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		memcpy(dest, &src, BC_DatatypeSize((BC_Datatype)ocx.datatype));
		return true;
	}
	bool Interpreter::exec_insAdd(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, +=, src, ocx.datatype);
		return true;
	}
	bool Interpreter::exec_insSubtract(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, -=, src, ocx.datatype);
		return true;
	}
	bool Interpreter::exec_insMultiply(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, *=, src, ocx.datatype);
		return true;
	}
	bool Interpreter::exec_insDivide(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, /=, src, ocx.datatype);
		return true;
	}
	bool Interpreter::exec_insConvert(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_Datatype newDT = readCodeAndMove<BC_Datatype>();

		// TODO: More compact
		#define COMB_DT(left, right) (((uint32_t)left << 16) | (uint32_t)right)
		switch (COMB_DT(ocx.datatype, newDT))
		{
		case COMB_DT(BC_DT_I_8,	 BC_DT_I_8):  dest.as_I_8  = dest.as_I_8;  break;
		case COMB_DT(BC_DT_I_8,	 BC_DT_I_16): dest.as_I_16 = dest.as_I_8;  break;
		case COMB_DT(BC_DT_I_8,	 BC_DT_I_32): dest.as_I_32 = dest.as_I_8;  break;
		case COMB_DT(BC_DT_I_8,	 BC_DT_I_64): dest.as_I_64 = dest.as_I_8;  break;
		case COMB_DT(BC_DT_I_8,	 BC_DT_U_8):  dest.as_U_8  = dest.as_I_8;  break;
		case COMB_DT(BC_DT_I_8,	 BC_DT_U_16): dest.as_U_16 = dest.as_I_8;  break;
		case COMB_DT(BC_DT_I_8,	 BC_DT_U_32): dest.as_U_32 = dest.as_I_8;  break;
		case COMB_DT(BC_DT_I_8,	 BC_DT_U_64): dest.as_U_64 = dest.as_I_8;  break;
		case COMB_DT(BC_DT_I_8,	 BC_DT_F_32): dest.as_F_32 = dest.as_I_8;  break;
		case COMB_DT(BC_DT_I_8,	 BC_DT_F_64): dest.as_F_64 = dest.as_I_8;  break;
		case COMB_DT(BC_DT_I_8,	 BC_DT_BOOL): dest.as_BOOL = dest.as_I_8;  break;

		case COMB_DT(BC_DT_I_16, BC_DT_I_8):  dest.as_I_8  = dest.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_I_16): dest.as_I_16 = dest.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_I_32): dest.as_I_32 = dest.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_I_64): dest.as_I_64 = dest.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_U_8):  dest.as_U_8  = dest.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_U_16): dest.as_U_16 = dest.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_U_32): dest.as_U_32 = dest.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_U_64): dest.as_U_64 = dest.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_F_32): dest.as_F_32 = dest.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_F_64): dest.as_F_64 = dest.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_BOOL): dest.as_BOOL = dest.as_I_16; break;

		case COMB_DT(BC_DT_I_32, BC_DT_I_8):  dest.as_I_8 =  dest.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_I_16): dest.as_I_16 = dest.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_I_32): dest.as_I_32 = dest.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_I_64): dest.as_I_64 = dest.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_U_8):  dest.as_U_8 =  dest.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_U_16): dest.as_U_16 = dest.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_U_32): dest.as_U_32 = dest.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_U_64): dest.as_U_64 = dest.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_F_32): dest.as_F_32 = dest.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_F_64): dest.as_F_64 = dest.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_BOOL): dest.as_BOOL = dest.as_I_32; break;

		case COMB_DT(BC_DT_I_64, BC_DT_I_8):  dest.as_I_8 =  dest.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_I_16): dest.as_I_16 = dest.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_I_32): dest.as_I_32 = dest.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_I_64): dest.as_I_64 = dest.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_U_8):  dest.as_U_8 =  dest.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_U_16): dest.as_U_16 = dest.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_U_32): dest.as_U_32 = dest.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_U_64): dest.as_U_64 = dest.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_F_32): dest.as_F_32 = dest.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_F_64): dest.as_F_64 = dest.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_BOOL): dest.as_BOOL = dest.as_I_64; break;

		case COMB_DT(BC_DT_U_8,  BC_DT_I_8):  dest.as_I_8 =  dest.as_U_8;  break;
		case COMB_DT(BC_DT_U_8,  BC_DT_I_16): dest.as_I_16 = dest.as_U_8;  break;
		case COMB_DT(BC_DT_U_8,  BC_DT_I_32): dest.as_I_32 = dest.as_U_8;  break;
		case COMB_DT(BC_DT_U_8,  BC_DT_I_64): dest.as_I_64 = dest.as_U_8;  break;
		case COMB_DT(BC_DT_U_8,  BC_DT_U_8):  dest.as_U_8 =  dest.as_U_8;  break;
		case COMB_DT(BC_DT_U_8,  BC_DT_U_16): dest.as_U_16 = dest.as_U_8;  break;
		case COMB_DT(BC_DT_U_8,  BC_DT_U_32): dest.as_U_32 = dest.as_U_8;  break;
		case COMB_DT(BC_DT_U_8,  BC_DT_U_64): dest.as_U_64 = dest.as_U_8;  break;
		case COMB_DT(BC_DT_U_8,  BC_DT_F_32): dest.as_F_32 = dest.as_U_8;  break;
		case COMB_DT(BC_DT_U_8,  BC_DT_F_64): dest.as_F_64 = dest.as_U_8;  break;
		case COMB_DT(BC_DT_U_8,  BC_DT_BOOL): dest.as_BOOL = dest.as_U_8;  break;

		case COMB_DT(BC_DT_U_16, BC_DT_I_8):  dest.as_I_8 =  dest.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_I_16): dest.as_I_16 = dest.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_I_32): dest.as_I_32 = dest.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_I_64): dest.as_I_64 = dest.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_U_8):  dest.as_U_8 =  dest.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_U_16): dest.as_U_16 = dest.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_U_32): dest.as_U_32 = dest.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_U_64): dest.as_U_64 = dest.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_F_32): dest.as_F_32 = dest.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_F_64): dest.as_F_64 = dest.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_BOOL): dest.as_BOOL = dest.as_U_16; break;

		case COMB_DT(BC_DT_U_32, BC_DT_I_8):  dest.as_I_8 =  dest.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_I_16): dest.as_I_16 = dest.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_I_32): dest.as_I_32 = dest.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_I_64): dest.as_I_64 = dest.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_U_8):  dest.as_U_8 =  dest.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_U_16): dest.as_U_16 = dest.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_U_32): dest.as_U_32 = dest.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_U_64): dest.as_U_64 = dest.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_F_32): dest.as_F_32 = dest.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_F_64): dest.as_F_64 = dest.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_BOOL): dest.as_BOOL = dest.as_U_32; break;

		case COMB_DT(BC_DT_U_64, BC_DT_I_8):  dest.as_I_8 =  dest.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_I_16): dest.as_I_16 = dest.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_I_32): dest.as_I_32 = dest.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_I_64): dest.as_I_64 = dest.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_U_8):  dest.as_U_8 =  dest.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_U_16): dest.as_U_16 = dest.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_U_32): dest.as_U_32 = dest.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_U_64): dest.as_U_64 = dest.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_F_32): dest.as_F_32 = dest.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_F_64): dest.as_F_64 = dest.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_BOOL): dest.as_BOOL = dest.as_U_64; break;

		case COMB_DT(BC_DT_F_32, BC_DT_I_8):  dest.as_I_8 =  dest.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_I_16): dest.as_I_16 = dest.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_I_32): dest.as_I_32 = dest.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_I_64): dest.as_I_64 = dest.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_U_8):  dest.as_U_8 =  dest.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_U_16): dest.as_U_16 = dest.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_U_32): dest.as_U_32 = dest.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_U_64): dest.as_U_64 = dest.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_F_32): dest.as_F_32 = dest.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_F_64): dest.as_F_64 = dest.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_BOOL): dest.as_BOOL = dest.as_F_32; break;

		case COMB_DT(BC_DT_F_64, BC_DT_I_8):  dest.as_I_8 =  dest.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_I_16): dest.as_I_16 = dest.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_I_32): dest.as_I_32 = dest.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_I_64): dest.as_I_64 = dest.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_U_8):  dest.as_U_8 =  dest.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_U_16): dest.as_U_16 = dest.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_U_32): dest.as_U_32 = dest.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_U_64): dest.as_U_64 = dest.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_F_32): dest.as_F_32 = dest.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_F_64): dest.as_F_64 = dest.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_BOOL): dest.as_BOOL = dest.as_F_64; break;

		case COMB_DT(BC_DT_BOOL, BC_DT_I_8):  dest.as_I_8 =  dest.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_I_16): dest.as_I_16 = dest.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_I_32): dest.as_I_32 = dest.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_I_64): dest.as_I_64 = dest.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_U_8):  dest.as_U_8 =  dest.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_U_16): dest.as_U_16 = dest.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_U_32): dest.as_U_32 = dest.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_U_64): dest.as_U_64 = dest.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_F_32): dest.as_F_32 = dest.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_F_64): dest.as_F_64 = dest.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_BOOL): dest.as_BOOL = dest.as_BOOL; break;
		}

		return true;
	}
	bool Interpreter::exec_insPush(BC_OpCodeEx ocx)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		regSP.as_ADDR.address += BC_DatatypeSize((BC_Datatype)ocx.datatype);

		return true;
	}
	bool Interpreter::exec_insPop(BC_OpCodeEx ocx)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		regSP.as_ADDR.address -= BC_DatatypeSize((BC_Datatype)ocx.datatype);

		return true;
	}
	bool Interpreter::exec_insPushCopy(BC_OpCodeEx ocx)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		auto dest = hostAddress(regSP.as_ADDR, false);
		auto src = &readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg0);

		memcpy(dest, src, BC_DatatypeSize((BC_Datatype)ocx.datatype));

		regSP.as_ADDR.address += BC_DatatypeSize((BC_Datatype)ocx.datatype);

		return true;
	}
	bool Interpreter::exec_insPopCopy(BC_OpCodeEx ocx)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		regSP.as_ADDR.address -= BC_DatatypeSize((BC_Datatype)ocx.datatype);

		auto src = hostAddress(regSP.as_ADDR, false);
		auto dest = hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);

		memcpy(dest, src, BC_DatatypeSize((BC_Datatype)ocx.datatype));

		return true;
	}
}