#include "Interpreter.h"

namespace MarC
{
	Interpreter::Interpreter(MemoryRef staticStack, MemoryRef codeMemory)
	{
		initMemory(staticStack, codeMemory);
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
		case BC_MEM_BASE_UNKNOWN:
			break;
		case BC_MEM_BASE_STATIC_STACK:
			hostAddr = (char*)m_mem.staticStack->getBaseAddress() + clientAddr.address;
			break;
		case BC_MEM_BASE_DYNAMIC_STACK:
			hostAddr = (char*)m_mem.dynamicStack->getBaseAddress() + clientAddr.address;
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

	void Interpreter::initMemory(MemoryRef staticStack, MemoryRef codeMemory)
	{
		if (staticStack)
			m_mem.staticStack = staticStack;
		if (codeMemory)
			m_mem.codeMemory = codeMemory;

		m_mem.dynamicStack.reset(new Memory);

		getRegister(BC_MEM_REG_STACK_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_DYNAMIC_STACK, 0);
		getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_DYNAMIC_STACK, 0);
		getRegister(BC_MEM_REG_LOOP_COUNTER).as_U_64 = 0;
		getRegister(BC_MEM_REG_ACCUMULATOR).as_U_64 = 0;
		getRegister(BC_MEM_REG_CODE_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_CODE_MEMORY, 0);
		getRegister(BC_MEM_REG_EXIT_CODE).as_U_64 = 0;
	}

	BC_MemCell Interpreter::readMemCellAndMove(BC_Datatype dt, bool deref)
	{
		return deref
			? *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), false)
			: readCodeAndMove<BC_MemCell>(BC_DatatypeSize(dt));
	}

	bool Interpreter::execNext()
	{
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
}