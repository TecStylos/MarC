#pragma once

#include "InterpreterMemory.h"

namespace MarC
{
	class Interpreter
	{
		static constexpr uint64_t RunTillEOC = -1; // Run until the interpreter reaches the end of code.
	public:
		Interpreter(MemoryRef staticStack, MemoryRef codeMemory, uint64_t dynStackSize);
	public:
		bool interpret(uint64_t nInstructinos = RunTillEOC);
	public:
		void* hostAddress(BC_MemAddress clientAddr, bool deref = false);
		BC_MemAddress clientAddress(void* hostAddr, BC_MemBase base);
		BC_MemCell& getRegister(BC_MemRegister reg);
	private:
		void initMemory(MemoryRef staticStack, MemoryRef codeMemory, uint64_t dynStackSize);
		template <typename T> T& readCodeAndMove();
		template <typename T> T& readCodeAndMove(uint64_t shift);
		BC_MemCell& readMemCellAndMove(BC_Datatype dt, bool deref);
		bool execNext();
		bool exec_insMove(BC_OpCodeEx ocx);
		bool exec_insAdd(BC_OpCodeEx ocx);
		bool exec_insSubtract(BC_OpCodeEx ocx);
		bool exec_insMultiply(BC_OpCodeEx ocx);
		bool exec_insDivide(BC_OpCodeEx ocx);
		bool exec_insConvert(BC_OpCodeEx ocx);
		bool exec_insPush(BC_OpCodeEx ocx);
		bool exec_insPop(BC_OpCodeEx ocx);
		bool exec_insPushCopy(BC_OpCodeEx ocx);
		bool exec_insPopCopy(BC_OpCodeEx ocx);
	private:
		InterpreterMemory m_mem;
	};

	template <typename T> T& Interpreter::readCodeAndMove()
	{
		return readCodeAndMove<T>(sizeof(T));
	}

	template <typename T> T& Interpreter::readCodeAndMove(uint64_t shift)
	{
		auto& val = *(T*)hostAddress(getRegister(BC_MEM_REG_CODE_POINTER).as_ADDR, false);
		getRegister(BC_MEM_REG_CODE_POINTER).as_ADDR.address += shift;
		return val;
	}
}

#define MARC_INTERPRETER_BINARY_OP(__left, __operator, __right, __datatype) \
switch (__datatype) { \
case MarC::BC_DT_I_8:  __left.as_I_8 __operator  __right.as_I_8;  break; \
case MarC::BC_DT_I_16: __left.as_I_16 __operator __right.as_I_16; break; \
case MarC::BC_DT_I_32: __left.as_I_32 __operator __right.as_I_32; break; \
case MarC::BC_DT_I_64: __left.as_I_64 __operator __right.as_I_64; break; \
\
case MarC::BC_DT_U_8:  __left.as_U_8  __operator __right.as_U_8;  break; \
case MarC::BC_DT_U_16: __left.as_U_16 __operator __right.as_U_16; break; \
case MarC::BC_DT_U_32: __left.as_U_32 __operator __right.as_U_32; break; \
case MarC::BC_DT_U_64: __left.as_U_64 __operator __right.as_U_64; break; \
\
case MarC::BC_DT_F_32: __left.as_F_32 __operator __right.as_F_32; break; \
case MarC::BC_DT_F_64: __left.as_F_64 __operator __right.as_F_64; break; \
\
case MarC::BC_DT_BOOL: __left.as_BOOL __operator __right.as_BOOL; break; \
}