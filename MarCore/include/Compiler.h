#pragma once

#include <string>
#include "Memory.h"
#include "CompilerTypes.h"

namespace MarC
{
	class CompilerError
	{
	public:
		enum class Code
		{
			Success = 0,
			OpCodeUnknown,
			OpCodeMissing,
			DatatypeUnknown,
			DatatypeMissing,
			DatatypeMismatch,
			TokenUnexpectedNum,
			LiteralMissingAfterDerefOp,
			NumericArgumentBroken,
			NumericLiteralBroken,
			CharInvalid,
			NotImplemented,
			DirectiveUnknown,
			LabelAlreadyDefined,
		};
	public:
		CompilerError() = default;
		CompilerError(Code code, uint64_t errLine, const std::string& errText, uint64_t sysErrLine, const std::string& sysErrFile)
			: m_code(code), m_errLine(errLine), m_errText(errText), m_sysErrLine(sysErrLine), m_sysErrFile(sysErrFile)
		{}
	public:
		operator bool() const;
		const std::string& getText() const;
		std::string getMessage() const;
	private:
		Code m_code = Code::Success;
		uint64_t m_errLine = 0;
		std::string m_errText = "Success!";
		uint64_t m_sysErrLine = 0;
		std::string m_sysErrFile = "";
	};

	typedef CompilerError::Code CompErrCode;

	#define RETURN_WITH_ERROR(errCode, errText) { m_lastErr = CompilerError(errCode, m_pModInfo->getErrLine(), errText, __LINE__, __FILE__); return false; }

	class Compiler
	{
	public:
		Compiler(const std::string& asmCode, MemoryRef staticStack);
	public:
		bool compile();
	public:
		ModuleInfoRef getModuleInfo();
	public:
		const CompilerError& lastError() const;
	private:
		;
	private:
		BC_MemAddress currCodeAddr();
	private:
		std::string m_asmCode;
		MemoryRef m_staticStack;
		CompilerError m_lastErr;
		ModuleInfoRef m_pModInfo;
		uint64_t m_nextCharToAssemble = 0;
	};
}