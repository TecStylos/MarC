#pragma once

#include <cstring>

#include "unused.h"
#include "InterpreterMemory.h"
#include "ExecutableInfo.h"
#include "ConvertInPlace.h"

#include "SearchAlgorithms.h"
#include "ExternalFunction.h"
#include "errors/InterpreterError.h"

namespace MarC
{

	typedef std::shared_ptr<class Interpreter> InterpreterRef;
	class Interpreter
	{
		static constexpr uint64_t RunTillEOC = -1; // Run until the interpreter reaches the end of code.
	public:
		Interpreter(ExecutableInfoRef pExeInfo, uint64_t defDynStackSize = 512);
	public:
		void addExtDir(const std::string& path);
	public:
		bool interpret(uint64_t nInstructinos = RunTillEOC);
	public:
		bool isGrantedPerm(const std::string& name) const;
		bool hasUngrantedPerms() const;
		bool hasUngrantedPerms(const std::set<std::string> perms) const;
		std::set<std::string> getUngrantedPerms(const std::set<std::string> perms) const;
		const std::set<std::string> getManPerms() const;
		const std::set<std::string> getOptPerms() const;
		void grantAllPerms();
		void grantPerm(const std::string& name);
		void grantPerms(const std::set<std::string>& names);
	public:
		void* getExternalAddress(BC_MemAddress exAddr);
		void* hostAddress(BC_MemAddress clientAddr);
		void* hostAddress(BC_MemAddress clientAddr, bool deref);
		template <typename T> T& hostObject(BC_MemAddress clientAddr);
		template <typename T> T& hostObject(BC_MemAddress clientAddr, bool deref);
		BC_MemCell& hostMemCell(BC_MemAddress clientAddr);
		BC_MemCell& hostMemCell(BC_MemAddress clientAddr, bool deref);
	public:
		BC_MemCell& getRegister(BC_MemRegister reg);
		const BC_MemCell& getRegister(BC_MemRegister reg) const;
		MarC::ExecutableInfoRef getExeInfo() const;
	public:
		uint64_t nInsExecuted() const;
	private:
		void initMemory(uint64_t dynStackSize);
		void loadMissingExtensions();
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
		void exec_insPushNBytes(BC_OpCodeEx ocx);
		void exec_insPopNBytes(BC_OpCodeEx ocx);
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
		void exec_insCallExtern(BC_OpCodeEx ocx);
		void exec_insCall(BC_OpCodeEx ocx);
		void exec_insReturn(BC_OpCodeEx ocx);
		void exec_insExit(BC_OpCodeEx ocx);
	private:
		void virt_pushStack(uint64_t nBytes);
		void virt_pushStack(const BC_MemCell& mc, uint64_t nBytes);
		void virt_popStack(uint64_t nBytes);
		void virt_popStack(BC_MemCell& mc, uint64_t nBytes);
		void virt_pushFrame();
		void virt_popFrame();
	private:
		ExternalFunctionPtr getExternalFunction(BC_MemAddress funcAddr);
	private:
		bool reachedEndOfCode() const;
	public:
		const InterpreterError& lastError() const;
		void resetError();
	public:
		static InterpreterRef create(ExecutableInfoRef pExeInfo, uint64_t defDynStackSize = 4096);
	private:
		ExecutableInfoRef m_pExeInfo;
		InterpreterMemory m_mem;
		std::map<BC_MemAddress, ExternalFunctionPtr> m_extFuncs;
		std::set<std::string> m_grantedPermissions;
		std::set<std::string> m_loadedExtensions;
		std::set<std::string> m_extDirs;
		InterpreterError m_lastErr;
		uint64_t m_nInsExecuted = 0;
	};

	template <typename T> T& Interpreter::hostObject(BC_MemAddress clientAddr)
	{
		return *(T*)hostAddress(clientAddr);
	}

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
		getRegister(BC_MEM_REG_CODE_POINTER).as_ADDR.addr += shift;
		return val;
	}

	inline void* Interpreter::getExternalAddress(BC_MemAddress exAddr)
	{
		auto& pair = findGreatestSmaller(exAddr, m_mem.dynMemMap);
		return (char*)pair.second + (exAddr.addr - pair.first.addr);
	}

	inline void* Interpreter::hostAddress(BC_MemAddress clientAddr)
	{
		return (clientAddr.base == BC_MEM_BASE_EXTERN) ?
			getExternalAddress(clientAddr) :
			(char*)m_mem.baseTable[clientAddr.base] + clientAddr.addr;
	}

	inline void* Interpreter::hostAddress(BC_MemAddress clientAddr, bool deref)
	{
		return deref ?
			hostAddress(*(BC_MemAddress*)hostAddress(clientAddr)) :
			hostAddress(clientAddr);
	}

	inline BC_MemCell& Interpreter::hostMemCell(BC_MemAddress clientAddr)
	{
		return hostObject<BC_MemCell>(clientAddr);
	}

	inline BC_MemCell& Interpreter::hostMemCell(BC_MemAddress clientAddr, bool deref)
	{
		return hostObject<BC_MemCell>(clientAddr, deref);
	}

	inline BC_MemCell& Interpreter::getRegister(BC_MemRegister reg)
	{
		return *(BC_MemCell*)((char*)&m_mem.registers + reg);
	}

	inline const BC_MemCell& Interpreter::getRegister(BC_MemRegister reg) const
	{
		return *(const BC_MemCell*)((const char*)&m_mem.registers + reg);
	}

	inline MarC::ExecutableInfoRef Interpreter::getExeInfo() const
	{
		return m_pExeInfo;
	}

	inline uint64_t Interpreter::nInsExecuted() const
	{
		return m_nInsExecuted;
	}

	inline BC_MemCell& Interpreter::readMemCellAndMove(BC_Datatype dt, bool deref)
	{
		return deref
			? hostMemCell(readDataAndMove<BC_MemAddress>())
			: readDataAndMove<BC_MemCell>(BC_DatatypeSize(dt));
	}

	inline void Interpreter::exec_insMove(BC_OpCodeEx ocx)
	{
		void* dest = hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		const void* src = &readMemCellAndMove(ocx.datatype, ocx.derefArg[1]);
		memcpy(dest, src, BC_DatatypeSize(ocx.datatype));
	}
	inline void Interpreter::exec_insPush(BC_OpCodeEx ocx)
	{
		virt_pushStack(BC_DatatypeSize(ocx.datatype));
	}
	inline void Interpreter::exec_insPop(BC_OpCodeEx ocx)
	{
		virt_popStack(BC_DatatypeSize(ocx.datatype));
	}
	inline void Interpreter::exec_insPushNBytes(BC_OpCodeEx ocx)
	{
		virt_pushStack(readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]).as_U_64);
	}
	inline void Interpreter::exec_insPopNBytes(BC_OpCodeEx ocx)
	{
		virt_popStack(readMemCellAndMove(BC_DT_U_64, ocx.derefArg[0]).as_U_64);
	}
	inline void Interpreter::exec_insPushCopy(BC_OpCodeEx ocx)
	{
		virt_pushStack(
			readMemCellAndMove(ocx.datatype, ocx.derefArg[0]),
			BC_DatatypeSize(ocx.datatype)
		);
	}
	inline void Interpreter::exec_insPopCopy(BC_OpCodeEx ocx)
	{
		virt_popStack(
			hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]),
			BC_DatatypeSize(ocx.datatype)
		);
	}
	inline void Interpreter::exec_insPushFrame(BC_OpCodeEx ocx)
	{
		UNUSED(ocx);
		virt_pushFrame();
	}
	inline void Interpreter::exec_insPopFrame(BC_OpCodeEx ocx)
	{
		UNUSED(ocx);
		virt_popFrame();
	}
	inline void Interpreter::exec_insDereference(BC_OpCodeEx ocx)
	{
		void* dest = hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		const void* src = hostAddress(readDataAndMove<BC_MemAddress>(), ocx.derefArg[1]);
		memcpy(dest, src, BC_DatatypeSize(ocx.datatype));
	}
	inline void Interpreter::exec_insConvert(BC_OpCodeEx ocx)
	{
		auto& mc = hostMemCell(readDataAndMove<BC_MemAddress>(), ocx.derefArg[0]);
		auto dt = readDataAndMove<BC_Datatype>();
		ConvertInPlace(mc, ocx.datatype, dt);
	}
	inline void Interpreter::exec_insJump(BC_OpCodeEx ocx)
	{
		getRegister(BC_MEM_REG_CODE_POINTER) = readMemCellAndMove(BC_DT_ADDR, ocx.derefArg[0]);
	}
	inline void Interpreter::exec_insReturn(BC_OpCodeEx ocx)
	{
		UNUSED(ocx);
		virt_popFrame();
		virt_popStack(
			getRegister(BC_MEM_REG_CODE_POINTER),
			BC_DatatypeSize(BC_DT_ADDR)
		);
	}

	inline void Interpreter::virt_pushStack(uint64_t nBytes)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		if (m_mem.dynamicStack.size() < regSP.as_ADDR.addr + nBytes)
		{
			m_mem.dynamicStack.resize(m_mem.dynamicStack.size() * 2);
			m_mem.baseTable[BC_MEM_BASE_DYNAMIC_STACK] = m_mem.dynamicStack.getBaseAddress();
			m_mem.baseTable[BC_MEM_BASE_DYNAMIC_FRAME] = (char*)m_mem.baseTable[BC_MEM_BASE_DYNAMIC_STACK] + getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR.addr;
		}

		regSP.as_ADDR.addr += nBytes;
	}

	inline void Interpreter::virt_pushStack(const BC_MemCell& mc, uint64_t nBytes)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		if (m_mem.dynamicStack.size() < regSP.as_ADDR.addr + nBytes)
		{
			m_mem.dynamicStack.resize(m_mem.dynamicStack.size() * 2);
			m_mem.baseTable[BC_MEM_BASE_DYNAMIC_STACK] = m_mem.dynamicStack.getBaseAddress();
			m_mem.baseTable[BC_MEM_BASE_DYNAMIC_FRAME] = (char*)m_mem.baseTable[BC_MEM_BASE_DYNAMIC_STACK] + getRegister(BC_MEM_REG_FRAME_POINTER).as_ADDR.addr;
		}
		
		auto dest = hostAddress(regSP.as_ADDR);

		memcpy(dest, &mc, nBytes);

		regSP.as_ADDR.addr += nBytes;
	}

	inline void Interpreter::virt_popStack(uint64_t nBytes)
	{
		getRegister(BC_MEM_REG_STACK_POINTER).as_ADDR.addr -= nBytes;
	}

	inline void Interpreter::virt_popStack(BC_MemCell& mc, uint64_t nBytes)
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);

		regSP.as_ADDR.addr -= nBytes;

		auto src = hostAddress(regSP.as_ADDR);

		memcpy(&mc, src, nBytes);
	}

	inline void Interpreter::virt_pushFrame()
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);
		auto& regFP = getRegister(BC_MEM_REG_FRAME_POINTER);
		virt_pushStack(
			regFP,
			BC_DatatypeSize(BC_DT_ADDR)
		);
		regFP.as_ADDR = regSP.as_ADDR;
		m_mem.baseTable[BC_MEM_BASE_DYNAMIC_FRAME] = (char*)m_mem.baseTable[BC_MEM_BASE_DYNAMIC_STACK] + regFP.as_ADDR.addr;
	}

	inline void Interpreter::virt_popFrame()
	{
		auto& regSP = getRegister(BC_MEM_REG_STACK_POINTER);
		auto& regFP = getRegister(BC_MEM_REG_FRAME_POINTER);
		regSP.as_ADDR = regFP.as_ADDR;
		virt_popStack(
			regFP,
			BC_DatatypeSize(BC_DT_ADDR)
		);
		m_mem.baseTable[BC_MEM_BASE_DYNAMIC_FRAME] = (char*)m_mem.baseTable[BC_MEM_BASE_DYNAMIC_STACK] + regFP.as_ADDR.addr;
	}

	inline bool Interpreter::reachedEndOfCode() const
	{
		return getRegister(BC_MEM_REG_CODE_POINTER).as_ADDR.addr >= (int64_t)m_mem.codeMemSize;
	}
}

#define MARC_INTERPRETER_BINARY_OP(__left, __operator, __right, __datatype) \
switch (__datatype) { \
case MarC::BC_DT_NONE: break; \
case MarC::BC_DT_UNKNOWN: break; \
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
case MarC::BC_DT_ADDR: __left.as_ADDR __operator __right.as_ADDR; break; \
case MarC::BC_DT_DATATYPE: break; \
}

#define MARC_INTERPRETER_BINARY_OP_BOOLEAN_RESULT(__result, __left, __operator, __right, __datatype) \
switch (__datatype) { \
case MarC::BC_DT_NONE: break; \
case MarC::BC_DT_UNKNOWN: break; \
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
\
case MarC::BC_DT_ADDR: __result = __left.as_ADDR __operator __right.as_ADDR; break; \
case MarC::BC_DT_DATATYPE: break; \
}
