#pragma once

#include "InterpreterMemory.h"
#include "LinkerOutput.h"

namespace MarC
{
	class InterpreterError
	{
	public:
		enum class Code
		{
			Success = 0,
			OpCodeUnknown,
			OpCodeNotExecutable,
			OpCodeNotImplemented,
			AbortViaExit,
			AbortViaEndOfCode,
		};
	public:
		InterpreterError() = default;
		InterpreterError(Code code, const std::string& errText)
			: m_code(code), m_errText(errText)
		{}
	public:
		operator bool() const;
		const std::string& getText() const;
		std::string getMessage() const;
		Code getCode() const;
		std::string getCodeStr() const;
		bool isOK() const;
	private:
		Code m_code = Code::Success;
		std::string m_errText = "Success!";
	};

	typedef InterpreterError::Code IntErrCode;

	class Interpreter
	{
		static constexpr uint64_t RunTillEOC = -1; // Run until the interpreter reaches the end of code.
	public:
		Interpreter(ExecutableInfoRef pExeInfo, uint64_t dynStackSize);
	public:
		bool interpret(uint64_t nInstructinos = RunTillEOC);
	public:
		void* hostAddress(BC_MemAddress clientAddr, bool deref = false);
		template <typename T> T& hostObject(BC_MemAddress clientAddr, bool deref = false);
		BC_MemCell& hostMemCell(BC_MemAddress clientAddr, bool deref = false);
	public:
		BC_MemCell& getRegister(BC_MemRegister reg);
		const BC_MemCell& getRegister(BC_MemRegister reg) const;
	public:
		uint64_t nInsExecuted() const;
	private:
		void initMemory(uint64_t dynStackSize);
		template <typename T> T& readDataAndMove();
		template <typename T> T& readDataAndMove(uint64_t shift);
		BC_MemCell& readMemCellAndMove(BC_Datatype dt, bool deref);
	private:
		void execNext();
		void exec_insUndefined(BC_OpCodeEx ocx);
		void exec_insMove(BC_OpCodeEx ocx);
		void exec_insAdd(BC_OpCodeEx ocx);
		void exec_insSubtract(BC_OpCodeEx ocx);
		void exec_insMultiply(BC_OpCodeEx ocx);
		void exec_insDivide(BC_OpCodeEx ocx);
		void exec_insDereference(BC_OpCodeEx ocx);
		void exec_insConvert(BC_OpCodeEx ocx);
		void exec_insPush(BC_OpCodeEx ocx);
		void exec_insPop(BC_OpCodeEx ocx);
		void exec_insPushCopy(BC_OpCodeEx ocx);
		void exec_insPopCopy(BC_OpCodeEx ocx);
		void exec_insPushFrame(BC_OpCodeEx ocx);
		void exec_insPopFrame(BC_OpCodeEx ocx);
		void exec_insJump(BC_OpCodeEx ocx);
		void exec_insJumpEqual(BC_OpCodeEx ocx);
		void exec_insJumpNotEqual(BC_OpCodeEx ocx);
		void exec_insJumpLessThan(BC_OpCodeEx ocx);
		void exec_insJumpGreaterThan(BC_OpCodeEx ocx);
		void exec_insJumpLessEqual(BC_OpCodeEx ocx);
		void exec_insJumpGreaterEqual(BC_OpCodeEx ocx);
		void exec_insAllocate(BC_OpCodeEx ocx);
		void exec_insFree(BC_OpCodeEx ocx);
		void exec_insCall(BC_OpCodeEx ocx);
		void exec_insReturn(BC_OpCodeEx ocx);
		void exec_insExit(BC_OpCodeEx ocx);
	private:
		void virt_pushStack(const BC_MemCell& mc, uint64_t nBytes);
		void virt_popStack(BC_MemCell& mc, uint64_t nBytes);
		void virt_pushFrame();
		void virt_popFrame();
	private:
		bool reachedEndOfCode() const;
	public:
		const InterpreterError& lastError() const;
	private:
		ExecutableInfoRef m_pExeInfo;
		InterpreterMemory m_mem;
		InterpreterError m_lastErr;
		uint64_t m_nInsExecuted = 0;
	};

	template <typename T> T& Interpreter::hostObject(BC_MemAddress clientAddr, bool deref)
	{
		return *(T*)hostAddress(clientAddr, deref);
	}

	template <typename T> T& Interpreter::readDataAndMove()
	{
		return readDataAndMove<T>(sizeof(T));
	}

	template <typename T> T& Interpreter::readDataAndMove(uint64_t shift)
	{
		auto& val = *(T*)hostAddress(getRegister(BC_MEM_REG_CODE_POINTER).as_ADDR, false);
		getRegister(BC_MEM_REG_CODE_POINTER).as_ADDR.asCode.addr += shift;
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
}

#define MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(__result, __left, __operator, __right, __datatype) \
switch (__datatype) { \
case MarC::BC_DT_I_8:  __result = __left.as_I_8 __operator  __right.as_I_8;  break; \
case MarC::BC_DT_I_16: __result = __left.as_I_16 __operator __right.as_I_16; break; \
case MarC::BC_DT_I_32: __result = __left.as_I_32 __operator __right.as_I_32; break; \
case MarC::BC_DT_I_64: __result = __left.as_I_64 __operator __right.as_I_64; break; \
\
case MarC::BC_DT_U_8:  __result = __left.as_U_8  __operator __right.as_U_8;  break; \
case MarC::BC_DT_U_16: __result = __left.as_U_16 __operator __right.as_U_16; break; \
case MarC::BC_DT_U_32: __result = __left.as_U_32 __operator __right.as_U_32; break; \
case MarC::BC_DT_U_64: __result = __left.as_U_64 __operator __right.as_U_64; break; \
\
case MarC::BC_DT_F_32: __result = __left.as_F_32 __operator __right.as_F_32; break; \
case MarC::BC_DT_F_64: __result = __left.as_F_64 __operator __right.as_F_64; break; \
}
