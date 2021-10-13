#include "Interpreter.h"

#include <algorithm>
#include <unordered_map>

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
			? *(BC_MemCell*)hostAddress(readDataAndMove<BC_MemAddress>(), false)
			: readDataAndMove<BC_MemCell>(BC_DatatypeSize(dt));
	}

	void Interpreter::execNext()
	{
		static const std::unordered_map<BC_OpCode, void (Interpreter::*)(BC_OpCodeEx)> opCodeFuns = {
			{ BC_OC_NONE, &Interpreter::exec_insMove },
			{ BC_OC_UNKNOWN, &Interpreter::exec_insMove },

			{ BC_OC_MOVE, &Interpreter::exec_insMove },
			{ BC_OC_ADD, &Interpreter::exec_insAdd },
			{ BC_OC_SUBTRACT, &Interpreter::exec_insSubtract },
			{ BC_OC_MULTIPLY, &Interpreter::exec_insMultiply },
			{ BC_OC_DIVIDE, &Interpreter::exec_insDivide },

			{ BC_OC_CONVERT, &Interpreter::exec_insConvert },

			{ BC_OC_PUSH, &Interpreter::exec_insPush },
			{ BC_OC_POP, &Interpreter::exec_insPop },
			{ BC_OC_PUSH_COPY, &Interpreter::exec_insPushCopy },
			{ BC_OC_POP_COPY, &Interpreter::exec_insPopCopy },

			{ BC_OC_PUSH_FRAME, &Interpreter::exec_insPushFrame },
			{ BC_OC_POP_FRAME, &Interpreter::exec_insPopFrame },

			{ BC_OC_JUMP, &Interpreter::exec_insJump },

			{ BC_OC_EXIT, &Interpreter::exec_insExit },
		};

		static bool noOpCodeMissing = []()
		{
			if (opCodeFuns.size() != BC_OC_NUM_OF_OP_CODES)
				throw InterpreterError(IntErrCode::OpCodeNotImplemented, "There are non-implemented opCodes!");
			return true;
		}();

		auto ocx = readDataAndMove<BC_OpCodeEx>();

		auto it = opCodeFuns.find((BC_OpCode)ocx.opCode);
		if (it == opCodeFuns.end())
			throw InterpreterError(IntErrCode::OpCodeNotExecutable, "Unknown opCode '" + std::to_string(ocx.opCode) + "'!");

		(*this.*(it->second))(ocx);
	}

	void Interpreter::exec_insMove(BC_OpCodeEx ocx)
	{
		void* dest = hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		memcpy(dest, &src, BC_DatatypeSize((BC_Datatype)ocx.datatype));
	}
	void Interpreter::exec_insAdd(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, +=, src, ocx.datatype);
	}
	void Interpreter::exec_insSubtract(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, -=, src, ocx.datatype);
	}
	void Interpreter::exec_insMultiply(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, *=, src, ocx.datatype);
	}
	void Interpreter::exec_insDivide(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_MemCell src = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg1);
		MARC_INTERPRETER_BINARY_OP(dest, /=, src, ocx.datatype);
	}
	void Interpreter::exec_insConvert(BC_OpCodeEx ocx)
	{
		auto& dest = *(BC_MemCell*)hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg0);
		BC_Datatype newDT = readDataAndMove<BC_Datatype>();

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
		BC_MemCell mc;
		virt_pushStack(mc, BC_DatatypeSize((BC_Datatype)ocx.datatype));
	}
	void Interpreter::exec_insPop(BC_OpCodeEx ocx)
	{
		BC_MemCell mc;
		virt_popStack(mc, BC_DatatypeSize((BC_Datatype)ocx.datatype));
	}
	void Interpreter::exec_insPushCopy(BC_OpCodeEx ocx)
	{
		BC_MemCell mc = readMemCellAndMove((BC_Datatype)ocx.datatype, ocx.derefArg0);
		virt_pushStack(mc, BC_DatatypeSize((BC_Datatype)ocx.datatype));
	}
	void Interpreter::exec_insPopCopy(BC_OpCodeEx ocx)
	{
		BC_MemCell mc = *(BC_MemCell*)hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg0);
		virt_popStack(mc, BC_DatatypeSize((BC_Datatype)ocx.datatype));
	}
	void Interpreter::exec_insPushFrame(BC_OpCodeEx ocx)
	{
		BC_MemCell& regFP = getRegister(BC_MEM_REG_FRAME_POINTER);
		virt_pushStack(regFP, BC_DatatypeSize(BC_DT_U_64));
		regFP = getRegister(BC_MEM_REG_STACK_POINTER);
	}
	void Interpreter::exec_insPopFrame(BC_OpCodeEx ocx)
	{
		BC_MemCell& regFP = getRegister(BC_MEM_REG_FRAME_POINTER);
		getRegister(BC_MEM_REG_STACK_POINTER) = regFP;
		virt_popStack(regFP, BC_DatatypeSize(BC_DT_U_64));
	}
	void Interpreter::exec_insJump(BC_OpCodeEx ocx)
	{
		auto& regCP = getRegister(BC_MEM_REG_CODE_POINTER);

		auto& destAddr = readMemCellAndMove(BC_DT_U_64, ocx.derefArg0);

		regCP = destAddr;
	}
	void Interpreter::exec_insExit(BC_OpCodeEx ocx)
	{
		throw InterpreterError(IntErrCode::AbortViaExit, "The program has been aborted with a call to exit!");
	}

	void Interpreter::virt_pushStack(const BC_MemCell& mc, uint64_t nBytes)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);
		
		auto dest = hostAddress(regSP.as_ADDR, false);

		memcpy(dest, &mc, nBytes);

		regSP.as_ADDR.addr += nBytes;
	}

	void Interpreter::virt_popStack(BC_MemCell& mc, uint64_t nBytes)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		regSP.as_ADDR.addr -= nBytes;

		auto src = hostAddress(regSP.as_ADDR, false);

		memcpy(&mc, src, nBytes);
	}

	const InterpreterError& Interpreter::lastError() const
	{
		return m_lastErr;
	}
}