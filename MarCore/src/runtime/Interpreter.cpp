#include "runtime/Interpreter.h"

#include <algorithm>
#include <unordered_map>

#include "fileio/ExtensionLocator.h"
#include "runtime/ExternalFunction.h"

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

		recalcExeMem();
		
		try
		{
			while (nInstructions--)
			{
				if (reachedEndOfCode())
					throw InterpreterError(IntErrCode::AbortViaEndOfCode, "EOC");

				const auto& ocx = readDataAndMove<BC_OpCodeEx>();

				switch (ocx.opCode)
				{
				case BC_OC_NONE:  exec_insUndefined(ocx); break;
				case BC_OC_UNKNOWN: exec_insUndefined(ocx); break;

				case BC_OC_MOVE: exec_insMove(ocx); break;
				case BC_OC_ADD: exec_insAdd(ocx); break;
				case BC_OC_SUBTRACT: exec_insSubtract(ocx); break;
				case BC_OC_MULTIPLY: exec_insMultiply(ocx); break;
				case BC_OC_DIVIDE: exec_insDivide(ocx); break;
				case BC_OC_INCREMENT: exec_insIncrement(ocx); break;
				case BC_OC_DECREMENT: exec_insDecrement(ocx); break;

				case BC_OC_CONVERT: exec_insConvert(ocx); break;

				case BC_OC_PUSH: exec_insPush(ocx); break;
				case BC_OC_POP: exec_insPop(ocx); break;
				case BC_OC_PUSH_N_BYTES: exec_insPushNBytes(ocx); break;
				case BC_OC_POP_N_BYTES: exec_insPopNBytes(ocx); break;
				case BC_OC_PUSH_COPY: exec_insPushCopy(ocx); break;
				case BC_OC_POP_COPY: exec_insPopCopy(ocx); break;

				case BC_OC_PUSH_FRAME: exec_insPushFrame(ocx); break;
				case BC_OC_POP_FRAME: exec_insPopFrame(ocx); break;

				case BC_OC_JUMP: exec_insJump(ocx); break;
				case BC_OC_JUMP_EQUAL: exec_insJumpEqual(ocx); break;
				case BC_OC_JUMP_NOT_EQUAL: exec_insJumpNotEqual(ocx); break;
				case BC_OC_JUMP_LESS_THAN: exec_insJumpLessThan(ocx); break;
				case BC_OC_JUMP_GREATER_THAN: exec_insJumpGreaterThan(ocx); break;
				case BC_OC_JUMP_LESS_EQUAL: exec_insJumpLessEqual(ocx); break;
				case BC_OC_JUMP_GREATER_EQUAL: exec_insJumpGreaterEqual(ocx); break;

				case BC_OC_ALLOCATE: exec_insAllocate(ocx); break;
				case BC_OC_FREE: exec_insFree(ocx); break;

				case BC_OC_CALL_EXTERN: exec_insCallExtern(ocx); break;

				case BC_OC_CALL: exec_insCall(ocx); break;
				case BC_OC_RETURN: exec_insReturn(ocx); break;

				case BC_OC_EXIT: exec_insExit(ocx); break;
				default:
					exec_insUndefined(ocx);
				}
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

	void Interpreter::initMemory(uint64_t dynStackSize)
	{
		m_mem.dynamicStack.resize(dynStackSize);

		m_mem.codeMemSize = m_pExeInfo->codeMemory.size();

		m_mem.baseTable[BC_MEM_BASE_NONE] = nullptr;
		m_mem.baseTable[BC_MEM_BASE_STATIC_STACK] = m_pExeInfo->staticStack.getBaseAddress();
		m_mem.baseTable[BC_MEM_BASE_DYNAMIC_STACK] = m_mem.dynamicStack.getBaseAddress();
		m_mem.baseTable[BC_MEM_BASE_DYNAMIC_FRAME] = (char*)m_mem.baseTable[BC_MEM_BASE_DYNAMIC_STACK] + getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR.addr;
		m_mem.baseTable[BC_MEM_BASE_CODE_MEMORY] = m_pExeInfo->codeMemory.getBaseAddress();
		m_mem.baseTable[BC_MEM_BASE_REGISTER] = &m_mem.registers;
		m_mem.baseTable[BC_MEM_BASE_EXTERN] = nullptr;

		getRegister(BC_MEM_REG_CODE_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_CODE_MEMORY, 0);
		getRegister(BC_MEM_REG_STACK_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_DYNAMIC_STACK, 0);
		getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR = BC_MemAddress(BC_MEM_BASE_DYNAMIC_FRAME, 0);
		getRegister(BC_MEM_REG_LOOP_COUNTER).as_U_64 = 0;
		getRegister(BC_MEM_REG_ACCUMULATOR).as_U_64 = 0;
		getRegister(BC_MEM_REG_TEMPORARY_DATA).as_U_64 = 0;
		getRegister(BC_MEM_REG_EXIT_CODE).as_U_64 = 0;
	}

	void Interpreter::recalcExeMem()
	{
		m_mem.codeMemSize = m_pExeInfo->codeMemory.size();
		m_mem.baseTable[BC_MEM_BASE_CODE_MEMORY] = m_pExeInfo->codeMemory.getBaseAddress();
		m_mem.baseTable[BC_MEM_BASE_STATIC_STACK] = m_pExeInfo->staticStack.getBaseAddress();
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
				continue;
			if (result.second.size() > 1)
				throw InterpreterError(IntErrCode::ExtensionLoadFailure, "Multiple extensions with name '" + result.first + "' found!");

			if (!PluS::PluginManager::get().loadPlugin(*result.second.begin()))
				throw InterpreterError(IntErrCode::ExtensionLoadFailure, "Unable to load extension '" + result.first + "'!");

			m_loadedExtensions.insert(result.first);
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
	void Interpreter::exec_insIncrement(BC_OpCodeEx ocx)
	{
		auto& dest = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		switch(ocx.datatype)
		{
		case BC_DT_NONE: break;
		case BC_DT_UNKNOWN: break;
		case BC_DT_I_8:  ++dest.as_I_8;  break;
		case BC_DT_I_16: ++dest.as_I_16; break;
		case BC_DT_I_32: ++dest.as_I_32; break;
		case BC_DT_I_64: ++dest.as_I_64; break;
		case BC_DT_U_8:  ++dest.as_U_8;  break;
		case BC_DT_U_16: ++dest.as_U_16; break;
		case BC_DT_U_32: ++dest.as_U_32; break;
		case BC_DT_U_64: ++dest.as_U_64; break;
		case BC_DT_F_32: dest.as_F_32 += 1.0f; break;
		case BC_DT_F_64: dest.as_F_64 += 1.0; break;
		case BC_DT_ADDR: ++dest.as_ADDR._raw; break;
		case BC_DT_DATATYPE: break;
		}
	}
	void Interpreter::exec_insDecrement(BC_OpCodeEx ocx)
	{
		auto& dest = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		switch(ocx.datatype)
		{
		case BC_DT_NONE: break;
		case BC_DT_UNKNOWN: break;
		case BC_DT_I_8:  --dest.as_I_8;  break;
		case BC_DT_I_16: --dest.as_I_16; break;
		case BC_DT_I_32: --dest.as_I_32; break;
		case BC_DT_I_64: --dest.as_I_64; break;
		case BC_DT_U_8:  --dest.as_U_8;  break;
		case BC_DT_U_16: --dest.as_U_16; break;
		case BC_DT_U_32: --dest.as_U_32; break;
		case BC_DT_U_64: --dest.as_U_64; break;
		case BC_DT_F_32: dest.as_F_32 -= 1.0f; break;
		case BC_DT_F_64: dest.as_F_64 -= 1.0; break;
		case BC_DT_ADDR: --dest.as_ADDR._raw; break;
		case BC_DT_DATATYPE: break;
		}
	}
	void Interpreter::exec_insJumpEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, == , rightOperand, ocx.datatype);

		getRegister(BC_MEM_REG_CODE_POINTER) = result ? destAddr : getRegister(BC_MEM_REG_CODE_POINTER);
	}
	void Interpreter::exec_insJumpNotEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, != , rightOperand, ocx.datatype);

		getRegister(BC_MEM_REG_CODE_POINTER) = result ? destAddr : getRegister(BC_MEM_REG_CODE_POINTER);
	}
	void Interpreter::exec_insJumpLessThan(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, < , rightOperand, ocx.datatype);

		getRegister(BC_MEM_REG_CODE_POINTER) = result ? destAddr : getRegister(BC_MEM_REG_CODE_POINTER);
	}
	void Interpreter::exec_insJumpGreaterThan(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, > , rightOperand, ocx.datatype);

		getRegister(BC_MEM_REG_CODE_POINTER) = result ? destAddr : getRegister(BC_MEM_REG_CODE_POINTER);
	}
	void Interpreter::exec_insJumpLessEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, <= , rightOperand, ocx.datatype);

		getRegister(BC_MEM_REG_CODE_POINTER) = result ? destAddr : getRegister(BC_MEM_REG_CODE_POINTER);
	}
	void Interpreter::exec_insJumpGreaterEqual(BC_OpCodeEx ocx)
	{
		auto& destAddr = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
		auto& leftOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		auto& rightOperand = readMemCellAndMove(ocx.datatype, ocx.derefArg[2]);

		bool result = false;

		MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(result, leftOperand, >= , rightOperand, ocx.datatype);

		getRegister(BC_MEM_REG_CODE_POINTER) = result ? destAddr : getRegister(BC_MEM_REG_CODE_POINTER);
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
	void Interpreter::exec_insExit(BC_OpCodeEx ocx)
	{
		UNUSED(ocx);
		throw InterpreterError(IntErrCode::AbortViaExit, "EXIT");
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
