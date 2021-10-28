#include "Interpreter.h"

#include <cstring>
#include <algorithm>
#include <unordered_map>
#include "ConvertInPlace.h"

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
				++m_nInsExecuted;
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
			hostAddr = (char*)m_pExeInfo->modules[clientAddr.asCode.page]->staticStack->getBaseAddress() + clientAddr.asCode.addr;
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
			hostAddr = &getRegister((BC_MemRegister)clientAddr.addr);
			break;
		case BC_MEM_BASE_EXTERN:
		  {
		    auto it = m_mem.dynMemMap.lower_bound(clientAddr);
		    if (it != m_mem.dynMemMap.begin())
		      --it;
		    hostAddr = (char*)it->second + (clientAddr.addr - it->first.addr);
		    break;
		  }
		}

		if (deref)
			hostAddr = hostAddress(*(BC_MemAddress*)hostAddr);
		return hostAddr;
	}

	BC_MemCell& Interpreter::hostMemCell(BC_MemAddress clientAddr, bool deref)
	{
		return hostObject<BC_MemCell>(clientAddr, deref);
	}

	BC_MemCell& Interpreter::getRegister(BC_MemRegister reg)
	{
		return m_mem.registers[reg];
	}

	uint64_t Interpreter::nInsExecuted() const
	{
		return m_nInsExecuted;
	}

	void Interpreter::initMemory(uint64_t dynStackSize)
	{
		m_mem.dynamicStack = std::make_shared<Memory>(dynStackSize);

		getRegister(BC_MEM_REG_CODE_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_CODE_MEMORY, 0);
		getRegister(BC_MEM_REG_STACK_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_DYNAMIC_STACK, 0);
		getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_DYNAMIC_STACK, 0);
		getRegister(BC_MEM_REG_LOOP_COUNTER).as_U_64 = 0;
		getRegister(BC_MEM_REG_ACCUMULATOR).as_U_64 = 0;
		getRegister(BC_MEM_REG_TEMPORARY_DATA).as_U_64 = 0;
		getRegister(BC_MEM_REG_EXIT_CODE).as_U_64 = 0;
	}

	BC_MemCell& Interpreter::readMemCellAndMove(BC_Datatype dt, bool deref)
	{
		return deref
			? hostMemCell(readDataAndMove<BC_MemAddress>(), false)
			: readDataAndMove<BC_MemCell>(BC_DatatypeSize(dt));
	}

	void Interpreter::execNext()
	{
		const auto& ocx = readDataAndMove<BC_OpCodeEx>();

		switch (ocx.opCode)
		{
		case BC_OC_NONE:  Interpreter::exec_insUndefined(ocx); break;
			case BC_OC_UNKNOWN: Interpreter::exec_insUndefined(ocx); break;

			case BC_OC_MOVE: Interpreter::exec_insMove(ocx); break;
			case BC_OC_ADD: Interpreter::exec_insAdd(ocx); break;
			case BC_OC_SUBTRACT: Interpreter::exec_insSubtract(ocx); break;
			case BC_OC_MULTIPLY: Interpreter::exec_insMultiply(ocx); break;
			case BC_OC_DIVIDE: Interpreter::exec_insDivide(ocx); break;

			case BC_OC_DEREFERENCE: Interpreter::exec_insDereference(ocx); break;

			case BC_OC_CONVERT: Interpreter::exec_insConvert(ocx); break;

			case BC_OC_PUSH: Interpreter::exec_insPush(ocx); break;
			case BC_OC_POP: Interpreter::exec_insPop(ocx); break;
			case BC_OC_PUSH_COPY: Interpreter::exec_insPushCopy(ocx); break;
			case BC_OC_POP_COPY: Interpreter::exec_insPopCopy(ocx); break;

			case BC_OC_PUSH_FRAME: Interpreter::exec_insPushFrame(ocx); break;
			case BC_OC_POP_FRAME: Interpreter::exec_insPopFrame(ocx); break;

			case BC_OC_JUMP: Interpreter::exec_insJump(ocx); break;
			case BC_OC_JUMP_EQUAL: Interpreter::exec_insJumpEqual(ocx); break;
			case BC_OC_JUMP_NOT_EQUAL: Interpreter::exec_insJumpNotEqual(ocx); break;
			case BC_OC_JUMP_LESS_THAN: Interpreter::exec_insJumpLessThan(ocx); break;
			case BC_OC_JUMP_GREATER_THAN: Interpreter::exec_insJumpGreaterThan(ocx); break;
			case BC_OC_JUMP_LESS_EQUAL: Interpreter::exec_insJumpLessEqual(ocx); break;
			case BC_OC_JUMP_GREATER_EQUAL: Interpreter::exec_insJumpGreaterEqual(ocx); break;

		case BC_OC_ALLOCATE: Interpreter::exec_insAllocate(ocx); break;
		case BC_OC_FREE: Interpreter::exec_insFree(ocx); break;
		  
		case BC_OC_CALL: Interpreter::exec_insCall(ocx); break;
			case BC_OC_RETURN: Interpreter::exec_insReturn(ocx); break;

			case BC_OC_EXIT: Interpreter::exec_insExit(ocx); break;
		default:
			throw InterpreterError(IntErrCode::OpCodeNotExecutable, "Unknown opCode '" + std::to_string(ocx.opCode) + "'!");
		}

	}

	void Interpreter::exec_insUndefined(BC_OpCodeEx ocx)
	{
		throw InterpreterError(IntErrCode::OpCodeUnknown, "Read undefined opCode '" + std::to_string(ocx.opCode) + "'!");
	}

	void Interpreter::exec_insMove(BC_OpCodeEx ocx)
	{
		void* dest = hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		auto& src = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		memcpy(dest, &src, BC_DatatypeSize(ocx.datatype));
	}
	void Interpreter::exec_insAdd(BC_OpCodeEx ocx)
	{
		auto& dest = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		auto& src = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		MARC_INTERPRETER_BINARY_OP(dest, +=, src, ocx.datatype);
	}
	void Interpreter::exec_insSubtract(BC_OpCodeEx ocx)
	{
		auto& dest = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		auto& src = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		MARC_INTERPRETER_BINARY_OP(dest, -=, src, ocx.datatype);
	}
	void Interpreter::exec_insMultiply(BC_OpCodeEx ocx)
	{
		auto& dest = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		auto& src = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		MARC_INTERPRETER_BINARY_OP(dest, *=, src, ocx.datatype);
	}
	void Interpreter::exec_insDivide(BC_OpCodeEx ocx)
	{
		auto& dest = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		auto& src = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		MARC_INTERPRETER_BINARY_OP(dest, /=, src, ocx.datatype);
	}
	void Interpreter::exec_insDereference(BC_OpCodeEx ocx)
	{
		void* dest = hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		void* src = hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg[1]);
		memcpy(dest, src, BC_DatatypeSize(BC_DT_U_64));
	}
	void Interpreter::exec_insConvert(BC_OpCodeEx ocx)
	{
		auto& dest = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		BC_Datatype dtNew = readDataAndMove<BC_Datatype>();
		ConvertInPlace(dest, ocx.datatype, dtNew);
	}
	void Interpreter::exec_insPush(BC_OpCodeEx ocx)
	{
		virt_pushStack(BC_MemCell(), BC_DatatypeSize(ocx.datatype));
	}
	void Interpreter::exec_insPop(BC_OpCodeEx ocx)
	{
	  BC_MemCell mc;
		virt_popStack(mc, BC_DatatypeSize(ocx.datatype));
	}
	void Interpreter::exec_insPushCopy(BC_OpCodeEx ocx)
	{
		auto& mc = readMemCellAndMove(ocx.datatype, ocx.derefArg[0]);
		virt_pushStack(mc, BC_DatatypeSize(ocx.datatype));
	}
	void Interpreter::exec_insPopCopy(BC_OpCodeEx ocx)
	{
		auto& mc = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		virt_popStack(mc, BC_DatatypeSize(ocx.datatype));
	}
	void Interpreter::exec_insPushFrame(BC_OpCodeEx ocx)
	{
		virt_pushFrame();
	}
	void Interpreter::exec_insPopFrame(BC_OpCodeEx ocx)
	{
		virt_popFrame();
	}
	void Interpreter::exec_insJump(BC_OpCodeEx ocx)
	{
		auto& regCP = getRegister(BC_MEM_REG_CODE_POINTER);

		auto& destAddr = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]);

		regCP = destAddr;
	}
	void Interpreter::exec_insJumpEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, == , rightOperand, ocx.datatype);

		if (result)
			getRegister(BC_MEM_REG_CODE_POINTER) = destAddr;
	}
	void Interpreter::exec_insJumpNotEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, != , rightOperand, ocx.datatype);

		if (result)
			getRegister(BC_MEM_REG_CODE_POINTER) = destAddr;
	}
	void Interpreter::exec_insJumpLessThan(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, < , rightOperand, ocx.datatype);

		if (result)
			getRegister(BC_MEM_REG_CODE_POINTER) = destAddr;
	}
	void Interpreter::exec_insJumpGreaterThan(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, > , rightOperand, ocx.datatype);

		if (result)
			getRegister(BC_MEM_REG_CODE_POINTER) = destAddr;
	}
	void Interpreter::exec_insJumpLessEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, <= , rightOperand, ocx.datatype);

		if (result)
			getRegister(BC_MEM_REG_CODE_POINTER) = destAddr;
	}
	void Interpreter::exec_insJumpGreaterEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, >= , rightOperand, ocx.datatype);

		if (result)
			getRegister(BC_MEM_REG_CODE_POINTER) = destAddr;
	}
  void Interpreter::exec_insAllocate(BC_OpCodeEx ocx)
  {
    BC_MemAddress& addr = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]).as_ADDR;
    uint64_t size = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[1]).as_U_64;
    addr.base = BC_MEM_BASE_EXTERN;
    addr.addr = m_mem.nextDynAddr;
    void* ptr = malloc(size);
    m_mem.dynMemMap.insert({ addr, ptr });
    m_mem.nextDynAddr += size;
  }
  void Interpreter::exec_insFree(BC_OpCodeEx ocx)
  {
    auto& addr = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]).as_ADDR;
    auto it = m_mem.dynMemMap.find(addr);
    if (it != m_mem.dynMemMap.end())
      free(it->second);
    m_mem.dynMemMap.erase(addr);
  }
  void Interpreter::exec_insCall(BC_OpCodeEx ocx)
	{
		BC_MemAddress fpMem;
		BC_MemAddress retMem;
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);
		auto& regFP = getRegister(BC_MEM_REG_FRAME_POINTER);
		auto& regCP = getRegister(BC_MEM_REG_CODE_POINTER);


		BC_MemAddress funcAddr = readMemCellAndMove(BC_DT_U_64, ocx.derefArg.get(0)).as_ADDR;
		auto& fcd = readDataAndMove<BC_FuncCallData>();
		
		virt_pushStack(BC_MemCell(), BC_DatatypeSize(ocx.datatype)); // Reserve memory for return value
		retMem = regSP.as_ADDR; // Copy address of memory for return address
		virt_pushStack(BC_MemCell(), BC_DatatypeSize(BC_DT_U_64)); // Reserve memory for return address
		fpMem = regSP.as_ADDR; // Copy address of memory for frame pointer
		virt_pushStack(BC_MemCell(), BC_DatatypeSize(BC_DT_U_64)); // Reserve memory for frame pointer

		for (uint64_t i = 0; i < fcd.nArgs; ++i)
		{
			bool deref = ocx.derefArg.get(1 + i);
			auto dt = fcd.argType.get(i);
			virt_pushStack(
				readMemCellAndMove(dt, deref),
				BC_DatatypeSize(dt)
			);
		}

		hostMemCell(fpMem, false).as_ADDR = regFP.as_ADDR; // Store the old frame pointer
		fpMem.addr += 8; // Frame pointer points to first byte after frame pointer backup
		regFP.as_ADDR = fpMem; // Initialize the new frame pointer

		hostMemCell(retMem, false).as_ADDR = regCP.as_ADDR; // Store the return address

		regCP.as_ADDR = funcAddr; // Jump to function address
	}
	void Interpreter::exec_insReturn(BC_OpCodeEx ocx)
	{
		virt_popFrame();
		auto& regCP = getRegister(BC_MEM_REG_CODE_POINTER);
		virt_popStack(regCP, BC_DatatypeSize(BC_DT_U_64));
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

	void Interpreter::virt_pushFrame()
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);
		auto& regFP = getRegister(BC_MEM_REG_FRAME_POINTER);
		virt_pushStack(regFP, BC_DatatypeSize(BC_DT_U_64));
		regFP.as_ADDR = regSP.as_ADDR;
	}

	void Interpreter::virt_popFrame()
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);
		auto& regFP = getRegister(BC_MEM_REG_FRAME_POINTER);
		regSP.as_ADDR = regFP.as_ADDR;
		virt_popStack(regFP, BC_DatatypeSize(BC_DT_U_64));
	}

	const InterpreterError& Interpreter::lastError() const
	{
		return m_lastErr;
	}
}
