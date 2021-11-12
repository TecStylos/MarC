#include "Interpreter.h"

#include <cstring>
#include <algorithm>
#include <unordered_map>
#include "ConvertInPlace.h"

#include "SearchAlgorithms.h"
#include "ExtensionLocator.h"
#include "ExternalFunction.h"

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
	std::string InterpreterError::getCodeStr() const
	{
		switch (getCode())
		{
		case IntErrCode::Success: return "Success";
		case IntErrCode::OpCodeUnknown: return "OpCode unknown";
		case IntErrCode::OpCodeNotExecutable: return "OpCode not executable";
		case IntErrCode::OpCodeNotImplemented: return "OpCode not implemented";
		case IntErrCode::AbortViaExit: return "Abort via exit";
		case IntErrCode::AbortViaEndOfCode: return "Abort via EndOfCode";
		}

		return "<unknown>";
	}
	bool InterpreterError::isOK() const
	{
		switch (m_code)
		{
		case Code::Success:
		case Code::AbortViaExit:
		case Code::AbortViaEndOfCode:
			return true;
		}

		return false;
	}

	Interpreter::Interpreter(ExecutableInfoRef pExeInfo, uint64_t defDynStackSize)
		: m_pExeInfo(pExeInfo)
	{
		initMemory(defDynStackSize);
	}

	void Interpreter::addExtDir(const std::string& path)
	{
		m_extDirs.insert(path);
	}

	bool Interpreter::interpret(uint64_t nInstructions)
	{
		resetError();

		if (!nInstructions)
			return true;

		try
		{
			while (nInstructions--)
			{
				execNext();
				++m_nInsExecuted;
			}
		}
		catch (const InterpreterError& ie)
		{
			m_lastErr = ie;
		}

		return !lastError();
	}
	const std::set<std::string> Interpreter::getManPerms() const
	{
		return m_pExeInfo->mandatoryPermissions;
	}
	const std::set<std::string> Interpreter::getOptPerms() const
	{
		return m_pExeInfo->optionalPermissions;
	}
	void Interpreter::grantAllPerms()
	{
		grantPerms(getManPerms());
		grantPerms(getOptPerms());
	}
	void Interpreter::grantPerm(const std::string& permission)
	{
		if (m_pExeInfo->mandatoryPermissions.find(permission) != m_pExeInfo->mandatoryPermissions.end() ||
			m_pExeInfo->optionalPermissions.find(permission) != m_pExeInfo->optionalPermissions.end()
			)
		{
			m_grantedPermissions.insert(permission);
		}
	}
	void Interpreter::grantPerms(const std::set<std::string>& permissions)
	{
		for (auto& perm : permissions)
			grantPerm(perm);
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
			auto& pair = findGreatestSmaller(clientAddr, m_mem.dynMemMap);
			hostAddr = (char*)pair.second + (clientAddr.addr - pair.first.addr);
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

	const BC_MemCell& Interpreter::getRegister(BC_MemRegister reg) const
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

	void Interpreter::loadMissingExtensions()
	{
		std::set<std::string> missingExtensions;

		for (auto& pModInfo : m_pExeInfo->modules)
			if (pModInfo->extensionRequired && !pModInfo->extensionLoaded)
				missingExtensions.insert(pModInfo->moduleName);

		auto results = locateExtensions(m_extDirs, missingExtensions);

		for (auto& result : results)
		{
			if (result.second.empty())
				throw InterpreterError(IntErrCode::ExtensionLoadFailure, "Extension '" + result.first + "' could not be found!");
			if (result.second.size() > 1)
				throw InterpreterError(IntErrCode::ExtensionLoadFailure, "Multiple extensions with name '" + result.first + "' found!");

			if (!PluS::PluginManager::get().loadPlugin(*result.second.begin()))
				throw InterpreterError(IntErrCode::ExtensionLoadFailure, "Unable to load extension '" + result.first + "'!");
		}

		for (auto& pModInfo : m_pExeInfo->modules)
			pModInfo->extensionLoaded = true;
	}

	BC_MemCell& Interpreter::readMemCellAndMove(BC_Datatype dt, bool deref)
	{
		return deref
			? hostMemCell(readDataAndMove<BC_MemAddress>(), false)
			: readDataAndMove<BC_MemCell>(BC_DatatypeSize(dt));
	}

	void Interpreter::execNext()
	{
		if (reachedEndOfCode())
			throw InterpreterError(IntErrCode::AbortViaEndOfCode, "Reached end of executable code!");

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
		case BC_OC_PUSH_N_BYTES: Interpreter::exec_insPushNBytes(ocx); break;
		case BC_OC_POP_N_BYTES: Interpreter::exec_insPopNBytes(ocx); break;
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

		case BC_OC_CALL_EXTERN: Interpreter::exec_insCallExtern(ocx); break;

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
	void Interpreter::exec_insPushNBytes(BC_OpCodeEx ocx)
	{
		uint16_t nBytes = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]).as_U_16;
		virt_pushStack(nBytes);
	}
	void Interpreter::exec_insPopNBytes(BC_OpCodeEx ocx)
	{
		uint16_t nBytes = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]).as_U_16;
		virt_popStack(nBytes);
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
		auto& addr = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]).as_ADDR;
		uint64_t size = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[1]).as_U_64;
		addr.base = BC_MEM_BASE_EXTERN;
		addr.addr = m_mem.nextDynAddr;
		void* ptr = malloc(size);
		m_mem.dynMemMap.insert({ addr, ptr });
		m_mem.nextDynAddr += size;
	}
	void Interpreter::exec_insFree(BC_OpCodeEx ocx)
	{
		auto& addr = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]).as_ADDR;
		auto it = m_mem.dynMemMap.find(addr);
		if (it != m_mem.dynMemMap.end())
			free(it->second);
		m_mem.dynMemMap.erase(addr);
	}
	void Interpreter::exec_insCallExtern(BC_OpCodeEx ocx)
	{
		uint64_t argIndex = 0;

		BC_MemAddress funcNameAddr = readMemCellAndMove(BC_DT_U_64, ocx.derefArg[argIndex++]).as_ADDR;
		auto& fcd = readDataAndMove<BC_FuncCallData>();

		ExternalFunctionPtr func = getExternalFunction(funcNameAddr);

		ExFuncData efd;
		efd.retVal.datatype = ocx.datatype;
		efd.nParams = fcd.nArgs;

		void* retDest = nullptr;
		if (ocx.datatype != BC_DT_NONE)
			hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg[argIndex++]);

		for (uint8_t i = 0; i < fcd.nArgs; ++i)
		{
			bool deref = ocx.derefArg.get(argIndex++);
			auto dt = fcd.argType.get(i);
			efd.param[i].datatype = dt;
			efd.param[i].cell = readMemCellAndMove(dt, deref);
		}

		func->call(*this, efd);

		if (retDest)
			memcpy(retDest, &efd.retVal.cell, BC_DatatypeSize(efd.retVal.datatype));
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

		for (uint8_t i = 0; i < fcd.nArgs; ++i)
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

	void Interpreter::virt_pushStack(uint64_t nBytes)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		if (m_mem.dynamicStack->size() < regSP.as_ADDR.addr + nBytes)
			m_mem.dynamicStack->resize(m_mem.dynamicStack->size() * 2);

		regSP.as_ADDR.addr += nBytes;
	}

	void Interpreter::virt_pushStack(const BC_MemCell& mc, uint64_t nBytes)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		if (m_mem.dynamicStack->size() < regSP.as_ADDR.addr + nBytes)
			m_mem.dynamicStack->resize(m_mem.dynamicStack->size() * 2);
		
		auto dest = hostAddress(regSP.as_ADDR, false);

		memcpy(dest, &mc, nBytes);

		regSP.as_ADDR.addr += nBytes;
	}

	void Interpreter::virt_popStack(uint64_t nBytes)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		regSP.as_ADDR.addr -= nBytes;
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

	ExternalFunctionPtr Interpreter::getExternalFunction(BC_MemAddress funcAddr)
	{
		auto funcIt = m_extFuncs.find(funcAddr);
		if (funcIt == m_extFuncs.end())
		{
			std::string funcName = &hostObject<char>(funcAddr);

			if (m_grantedPermissions.find(funcName) == m_grantedPermissions.end())
				throw InterpreterError(IntErrCode::PermissionDenied, "Insufficient permissions for external function '" + funcName + "'!");

			loadMissingExtensions();

			auto uid = PluS::PluginManager::get().findFeature(funcName);
			if (!uid)
				throw InterpreterError(IntErrCode::ExternalFunctionNotFound, "Unable to locate external function '" + funcName + "'!");
			ExternalFunctionPtr exFunc = PluS::PluginManager::get().createFeature<ExternalFunction>(uid);
			funcIt = m_extFuncs.insert({ funcAddr, exFunc }).first;
		}

		return funcIt->second;
	}

	bool Interpreter::reachedEndOfCode() const
	{
		auto& cp = getRegister(BC_MEM_REG_CODE_POINTER);
		auto addr = cp.as_ADDR.asCode.addr;
		auto& mod = *m_pExeInfo->modules[cp.as_ADDR.asCode.page];
		auto& code = *mod.codeMemory;

		return addr >= code.size();
	}

	const InterpreterError& Interpreter::lastError() const
	{
		return m_lastErr;
	}

	void Interpreter::resetError()
	{
		m_lastErr = InterpreterError();
	}
}
