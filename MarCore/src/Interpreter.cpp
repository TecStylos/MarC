#include "Interpreter.h"

#include <algorithm>
#include <unordered_map>
#include "ConvertInPlace.h"

#include "SearchAlgorithms.h"
#include "ExtensionLocator.h"
#include "ExternalFunction.h"

namespace MarC
{
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

	bool Interpreter::isGrantedPerm(const std::string& name) const
	{
		return m_grantedPermissions.find(name) != m_grantedPermissions.end();
	}
	bool Interpreter::hasUngrantedPerms() const
	{
		return m_grantedPermissions.size() < getManPerms().size() + getOptPerms().size();
	}
	bool Interpreter::hasUngrantedPerms(const std::set<std::string> perms) const
	{
		for (auto& perm : perms)
			if (!isGrantedPerm(perm))
				return true;
		return false;
	}
	std::set<std::string> Interpreter::getUngrantedPerms(const std::set<std::string> perms) const
	{
		std::set<std::string> ungrantedPerms;
		for (auto& perm : perms)
			if (!isGrantedPerm(perm))
				ungrantedPerms.insert(perm);
		return ungrantedPerms;
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
		if (!hasUngrantedPerms())
			return;
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

	void* Interpreter::hostAddress(BC_MemAddress clientAddr)
	{
		switch (clientAddr.base)
		{
		case BC_MEM_BASE_NONE:
			return nullptr;
		case BC_MEM_BASE_STATIC_STACK:
			return (char*)m_pExeInfo->staticStack.getBaseAddress() + clientAddr.addr;
		case BC_MEM_BASE_DYNAMIC_STACK:
			return (char*)m_mem.dynamicStackBase + clientAddr.addr;
		case BC_MEM_BASE_DYN_FRAME_ADD:
			return (char*)m_mem.dynamicStackBase + getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR.addr + clientAddr.addr;
		case BC_MEM_BASE_DYN_FRAME_SUB:
			return (char*)m_mem.dynamicStackBase + getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR.addr - clientAddr.addr;
		case BC_MEM_BASE_CODE_MEMORY:
			return (char*)m_mem.codeMemBase + clientAddr.addr;
		case BC_MEM_BASE_REGISTER:
			return &getRegister((BC_MemRegister)clientAddr.addr);
		case BC_MEM_BASE_EXTERN:
		{
			auto& pair = findGreatestSmaller(clientAddr, m_mem.dynMemMap);
			return (char*)pair.second + (clientAddr.addr - pair.first.addr);
		}
		}
		return nullptr;
	}

	void Interpreter::initMemory(uint64_t dynStackSize)
	{
		m_mem.dynamicStack.resize(dynStackSize);
		m_mem.dynamicStackBase = m_mem.dynamicStack.getBaseAddress();
		m_mem.codeMemBase = m_pExeInfo->codeMemory.getBaseAddress();

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

		for (auto& ext : m_pExeInfo->requiredExtensions)
			if (m_loadedExtensions.find(ext) == m_loadedExtensions.end())
				missingExtensions.insert(ext);

		auto results = locateExtensions(m_extDirs, missingExtensions);

		for (auto& result : results)
		{
			if (result.second.empty())
				throw InterpreterError(IntErrCode::ExtensionLoadFailure, "Extension '" + result.first + "' could not be found!");
			if (result.second.size() > 1)
				throw InterpreterError(IntErrCode::ExtensionLoadFailure, "Multiple extensions with name '" + result.first + "' found!");

			if (!PluS::PluginManager::get().loadPlugin(*result.second.begin()))
				throw InterpreterError(IntErrCode::ExtensionLoadFailure, "Unable to load extension '" + result.first + "'!");

			m_loadedExtensions.insert(result.first);
		}
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
			exec_insUndefined(ocx);
		}
	}

	void Interpreter::exec_insUndefined(BC_OpCodeEx ocx)
	{
		throw InterpreterError(IntErrCode::OpCodeUnknown, std::to_string(ocx.opCode));
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
		const void* src = hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg[1]);
		memcpy(dest, src, BC_DatatypeSize(ocx.datatype));
	}
	void Interpreter::exec_insConvert(BC_OpCodeEx ocx)
	{
		auto& mc = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		auto dt = readDataAndMove<BC_Datatype>();
		ConvertInPlace(mc, ocx.datatype, dt);
	}
	void Interpreter::exec_insJumpEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, == , rightOperand, ocx.datatype);

		if (result)
			getRegister(BC_MEM_REG_CODE_POINTER) = destAddr;
	}
	void Interpreter::exec_insJumpNotEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, != , rightOperand, ocx.datatype);

		if (result)
			getRegister(BC_MEM_REG_CODE_POINTER) = destAddr;
	}
	void Interpreter::exec_insJumpLessThan(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, < , rightOperand, ocx.datatype);

		if (result)
			getRegister(BC_MEM_REG_CODE_POINTER) = destAddr;
	}
	void Interpreter::exec_insJumpGreaterThan(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, > , rightOperand, ocx.datatype);

		if (result)
			getRegister(BC_MEM_REG_CODE_POINTER) = destAddr;
	}
	void Interpreter::exec_insJumpLessEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, <= , rightOperand, ocx.datatype);

		if (result)
			getRegister(BC_MEM_REG_CODE_POINTER) = destAddr;
	}
	void Interpreter::exec_insJumpGreaterEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
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
		uint64_t size = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[1]).as_U_64;
		addr.base = BC_MEM_BASE_EXTERN;
		addr.addr = m_mem.nextDynAddr;
		void* ptr = malloc(size);
		m_mem.dynMemMap.insert({ addr, ptr });
		m_mem.nextDynAddr += size;
	}
	void Interpreter::exec_insFree(BC_OpCodeEx ocx)
	{
		auto& addr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]).as_ADDR;
		auto it = m_mem.dynMemMap.find(addr);
		if (it != m_mem.dynMemMap.end())
			free(it->second);
		m_mem.dynMemMap.erase(addr);
	}
	void Interpreter::exec_insCallExtern(BC_OpCodeEx ocx)
	{
		uint64_t argIndex = 0;

		BC_MemAddress funcNameAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[argIndex++]).as_ADDR;
		auto& fcd = readDataAndMove<BC_FuncCallData>();

		ExternalFunctionPtr func = getExternalFunction(funcNameAddr);

		ExFuncData efd;
		efd.retVal.datatype = ocx.datatype;
		efd.nParams = fcd.nArgs;

		void* retDest = nullptr;
		if (ocx.datatype != BC_DT_NONE)
			retDest = hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg[argIndex++]);

		for (uint8_t i = 0; i < fcd.nArgs; ++i)
		{
			auto dt = fcd.argType.get(i);
			efd.param[i].datatype = dt;
			efd.param[i].cell = readMemCellAndMove(dt, ocx.derefArg.get(argIndex++));
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

		BC_MemAddress funcAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg.get(0)).as_ADDR;
		auto& fcd = readDataAndMove<BC_FuncCallData>();
		
		virt_pushStack(BC_DatatypeSize(ocx.datatype)); // Reserve memory for return value
		retMem = regSP.as_ADDR; // Copy address of memory for return address
		virt_pushStack(BC_DatatypeSize(BC_DT_ADDR)); // Reserve memory for return address
		fpMem = regSP.as_ADDR; // Copy address of memory for frame pointer
		virt_pushStack(BC_DatatypeSize(BC_DT_ADDR)); // Reserve memory for frame pointer

		for (uint8_t i = 0; i < fcd.nArgs; ++i)
		{
			bool deref = ocx.derefArg.get(1 + (uint64_t)i);
			auto dt = fcd.argType.get(i);
			virt_pushStack(
				readMemCellAndMove(dt, deref),
				BC_DatatypeSize(dt)
			);
		}

		hostMemCell(fpMem).as_ADDR = regFP.as_ADDR; // Store the old frame pointer
		fpMem.addr += 8; // Frame pointer points to first byte after frame pointer backup
		regFP.as_ADDR = fpMem; // Initialize the new frame pointer

		hostMemCell(retMem).as_ADDR = regCP.as_ADDR; // Store the return address

		regCP.as_ADDR = funcAddr; // Jump to function address
	}
	void Interpreter::exec_insReturn(BC_OpCodeEx ocx)
	{
		UNUSED(ocx);
		virt_popFrame();
		virt_popStack(
			getRegister(BC_MEM_REG_CODE_POINTER),
			BC_DatatypeSize(BC_DT_ADDR)
		);
	}
	void Interpreter::exec_insExit(BC_OpCodeEx ocx)
	{
		UNUSED(ocx);
		throw InterpreterError(IntErrCode::AbortViaExit, "The program has been aborted with a call to exit!");
	}

	void Interpreter::virt_popStack(uint64_t nBytes)
	{
		getRegister(BC_MEM_REG_STACK_POINTER).as_ADDR.addr -= nBytes;
	}

	void Interpreter::virt_popStack(BC_MemCell& mc, uint64_t nBytes)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		regSP.as_ADDR.addr -= nBytes;

		auto src = hostAddress(regSP.as_ADDR);

		memcpy(&mc, src, nBytes);
	}

	ExternalFunctionPtr Interpreter::getExternalFunction(BC_MemAddress funcAddr)
	{
		auto funcIt = m_extFuncs.find(funcAddr);
		if (funcIt == m_extFuncs.end())
		{
			std::string funcName = &hostObject<char>(funcAddr);

			if (m_grantedPermissions.find(funcName) == m_grantedPermissions.end())
				throw InterpreterError(IntErrCode::PermissionDenied, funcName);

			loadMissingExtensions();

			auto uid = PluS::PluginManager::get().findFeature(funcName);
			if (!uid)
				throw InterpreterError(IntErrCode::ExternalFunctionNotFound, funcName);
			ExternalFunctionPtr exFunc = PluS::PluginManager::get().createFeature<ExternalFunction>(uid);
			funcIt = m_extFuncs.insert({ funcAddr, exFunc }).first;
		}

		return funcIt->second;
	}

	const InterpreterError& Interpreter::lastError() const
	{
		return m_lastErr;
	}

	void Interpreter::resetError()
	{
		m_lastErr = InterpreterError();
	}

	InterpreterRef Interpreter::create(ExecutableInfoRef pExeInfo, uint64_t defDynStackSize)
	{
		return std::make_shared<Interpreter>(pExeInfo, defDynStackSize);
	}
}
