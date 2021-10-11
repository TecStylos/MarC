#include "Interpreter.h"

namespace MarC
{
	InterpreterError::operator bool() const
	{
		return m_code != Code::Success;
	}
	const std::string& InterpreterError::getText() const
	{
		return m_errText;
	}
	std::string InterpreterError::getMessage() const
	{
		return "Error with code " + std::to_string((uint64_t)m_code) + ": " + getText();
	}
	InterpreterError::Code InterpreterError::getCode() const
	{
		return m_code;
	}
	bool InterpreterError::isOK() const
	{
		return
			m_code == Code::Success ||
			m_code == Code::AbortViaExit;
	}

	Interpreter::Interpreter(ExecutableInfoRef pExeInfo, uint64_t dynStackSize)
		: m_pExeInfo(pExeInfo)
	{
		initMemory(dynStackSize);
	}

	bool Interpreter::interpret(uint64_t nInstructions)
	{
		m_lastErr = InterpreterError();

		if (!nInstructions)
			return true;

		while (nInstructions--)
		{
			try
			{
				execNext();
			}
			catch (const InterpreterError& ie)
			{
				m_lastErr = ie;
				break;
			}
		}

		return !lastError();
	}

	void* Interpreter::hostAddress(BC_MemAddress clientAddr, bool deref)
	{
		void* hostAddr = nullptr;

		switch (clientAddr.base)
		{
		case BC_MEM_BASE_NONE:
			break;
		case BC_MEM_BASE_STATIC_STACK:
			hostAddr = (char*)m_pExeInfo->staticStack->getBaseAddress() + clientAddr.addr;
			break;
		case BC_MEM_BASE_DYNAMIC_STACK:
			hostAddr = (char*)m_mem.dynamicStack->getBaseAddress() + clientAddr.addr;
			break;
		case BC_MEM_BASE_DYN_FRAME_ADD:
			hostAddr = (char*)hostAddress(getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR) + clientAddr.addr;
			break;
		case BC_MEM_BASE_DYN_FRAME_SUB:
			hostAddr = (char*)hostAddress(getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR) - clientAddr.addr;
			break;
		case BC_MEM_BASE_CODE_MEMORY:
			hostAddr = (char*)m_pExeInfo->modules[clientAddr.asCode.page]->codeMemory->getBaseAddress() + clientAddr.asCode.addr;
			break;
		case BC_MEM_BASE_REGISTER:
			hostAddr = &m_mem.registers[clientAddr.addr];
			break;
		case BC_MEM_BASE_EXTERN:
			hostAddr = (char*)0 + clientAddr.addr;
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

	void Interpreter::initMemory(uint64_t dynStackSize)
	{
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

	void Interpreter::execNext()
	{
		static_assert(BC_OC_NUM_OF_OP_CODES == 18);

		auto ocx = readCodeAndMove<BC_OpCodeEx>();

		switch (ocx.opCode)
		{
		//case BC_OC_NONE:
		//case BC_OC_UNKNOWN:
		//	throw InterpreterError(IntErrCode::OpCodeNotExecutable, "Unable to execute opCode '" + std::to_string(ocx.opCode) + "'!");
		case BC_OC_MOVE:
			exec_insMove(ocx); break;
		case BC_OC_ADD:
			exec_insAdd(ocx); break;
		case BC_OC_SUBTRACT:
			exec_insSubtract(ocx); break;
		case BC_OC_MULTIPLY:
			exec_insMultiply(ocx); break;
		case BC_OC_DIVIDE:
			exec_insDivide(ocx); break;

		case BC_OC_CONVERT:
			exec_insConvert(ocx); break;

		case BC_OC_COPY:
			throw InterpreterError(IntErrCode::OpCodeNotImplemented, "The opCode '" + std::to_string(ocx.opCode) + "' has not been implemented yet!");

		case BC_OC_PUSH:
			exec_insPush(ocx); break;
		case BC_OC_POP:
			exec_insPop(ocx); break;
		case BC_OC_PUSHC:
			exec_insPushCopy(ocx); break;
		case BC_OC_POPC:
			exec_insPopCopy(ocx); break;

		case BC_OC_PUSH_FRAME:
			exec_insPushFrame(ocx); break;
		case BC_OC_POP_FRAME:
			exec_insPopFrame(ocx); break;

		case BC_OC_CALL:
			throw InterpreterError(IntErrCode::OpCodeNotImplemented, "The opCode '" + std::to_string(ocx.opCode) + "' has not been implemented yet!");
		case BC_OC_RETURN:
			throw InterpreterError(IntErrCode::OpCodeNotImplemented, "The opCode '" + std::to_string(ocx.opCode) + "' has not been implemented yet!");

		case BC_OC_EXIT:
			throw InterpreterError(IntErrCode::AbortViaExit, "The program has been aborted with a call to exit!");
		default:
			throw InterpreterError(IntErrCode::OpCodeNotExecutable, "Unknown opCode '" + std::to_string(ocx.opCode) + "'!");
		}

	}

	void Interpreter::exec_insMove(BC_OpCodeEx ocx)
	{
		void* dest = hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		memcpy(dest, &src, BC_DatatypeSize((BC_Datatype)ocx.datatype));
	}
	void Interpreter::exec_insAdd(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, +=, src, ocx.datatype);
	}
	void Interpreter::exec_insSubtract(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, -=, src, ocx.datatype);
	}
	void Interpreter::exec_insMultiply(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, *=, src, ocx.datatype);
	}
	void Interpreter::exec_insDivide(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, /=, src, ocx.datatype);
	}
	void Interpreter::exec_insConvert(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_Datatype newDT = readCodeAndMove<BC_Datatype>();

		// TODO: More compact
		#pragma warning(disable: 4244)
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
		#undef COMB_DT
		#pragma warning(default: 4244)
	}
	void Interpreter::exec_insPush(BC_OpCodeEx ocx)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		regSP.as_ADDR.addr += BC_DatatypeSize((BC_Datatype)ocx.datatype);
	}
	void Interpreter::exec_insPop(BC_OpCodeEx ocx)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		regSP.as_ADDR.addr -= BC_DatatypeSize((BC_Datatype)ocx.datatype);
	}
	void Interpreter::exec_insPushCopy(BC_OpCodeEx ocx)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		auto dest = hostAddress(regSP.as_ADDR, false);
		auto src = &readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg0);

		memcpy(dest, src, BC_DatatypeSize((BC_Datatype)ocx.datatype));

		regSP.as_ADDR.addr += BC_DatatypeSize((BC_Datatype)ocx.datatype);
	}
	void Interpreter::exec_insPopCopy(BC_OpCodeEx ocx)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		regSP.as_ADDR.addr -= BC_DatatypeSize((BC_Datatype)ocx.datatype);

		auto src = hostAddress(regSP.as_ADDR, false);
		auto dest = hostAddress(readCodeAndMove<BC_MemAddress>(), ocx.derefArg0);

		memcpy(dest, src, BC_DatatypeSize((BC_Datatype)ocx.datatype));
	}
	void Interpreter::exec_insPushFrame(BC_OpCodeEx ocx)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);
		auto& regFP = getRegister(BC_MEM_REG_FRAME_POINTER);
		auto& oldFP = *(BC_MemCell*)hostAddress(regSP.as_ADDR, false);

		oldFP.as_ADDR = regFP.as_ADDR;
		regSP.as_ADDR.addr += BC_DatatypeSize(BC_DT_U_64);
		regFP.as_ADDR = regSP.as_ADDR;
	}
	void Interpreter::exec_insPopFrame(BC_OpCodeEx ocx)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);
		auto& regFP = getRegister(BC_MEM_REG_FRAME_POINTER);

		regSP.as_ADDR = regFP.as_ADDR;
		regSP.as_ADDR.addr -= BC_DatatypeSize(BC_DT_U_64);
		auto oldFP = *(BC_MemCell*)hostAddress(regSP.as_ADDR, false);
		regFP.as_ADDR = oldFP.as_ADDR;
	}

	const InterpreterError& Interpreter::lastError() const
	{
		return m_lastErr;
	}
}