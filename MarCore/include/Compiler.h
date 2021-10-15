#pragma once

#include <string>
#include "Memory.h"
#include "CompilerTypes.h"
#include "AsmInstructions.h"
#include "AsmTokenizerTypes.h"

namespace MarC
{
	class CompilerError
	{
	public:
		enum class Code
		{
			Success = 0,
			UnexpectedToken,
			UnknownInsArgType,
			DatatypeMismatch,
			UnknownRegisterName,
			UnknownDirectiveID,
			SymbolAlreadyDefined,
		};
	public:
		CompilerError() = default;
		CompilerError(Code code, uint64_t errToken, const std::string& errText, uint64_t sysErrLine, const std::string& sysErrFile)
			: m_code(code), m_errToken(errToken), m_errText(errText), m_sysErrLine(sysErrLine), m_sysErrFile(sysErrFile)
		{}
	public:
		operator bool() const;
		const std::string& getText() const;
		std::string getMessage() const;
	private:
		Code m_code = Code::Success;
		uint64_t m_errToken = 0;
		std::string m_errText = "Success!";
		uint64_t m_sysErrLine = 0;
		std::string m_sysErrFile = "<unspecified>";
	};

	typedef CompilerError::Code CompErrCode;

	#define COMPILER_RETURN_WITH_ERROR(errCode, errText) { m_lastErr = CompilerError(errCode, m_nextTokenToCompile, errText, __LINE__, __FILE__); return false; }

	class Compiler
	{
	public:
		Compiler(const AsmTokenListRef tokenList, MemoryRef staticStack);
	public:
		bool compile();
	public:
		ModuleInfoRef getModuleInfo();
	public:
		const CompilerError& lastError() const;
	private:
		bool compileStatement();
		bool compileInstruction();
	private:
		bool compileArgument(BC_OpCodeEx& ocx, const InsArgument& arg);
		bool compileArgAddress(BC_OpCodeEx& ocx, const InsArgument& arg);
		bool compileArgValue(BC_OpCodeEx& ocx, const InsArgument& arg);
		bool compileArgDatatype(BC_OpCodeEx& ocx, const InsArgument& arg);
	private:
		bool compileDirective();
		bool compileDirLabel();
	private:
		bool removeNecessaryColon();
	private:
		bool addSymbol(const std::string& name, const Symbol& symbol);
	private:
		bool isInstructionLike();
		bool isDirectiveLike();
	private:
		const AsmToken& nextToken();
		const AsmToken& currToken() const;
		bool isEndOfCode() const;
	private:
		void pushCode(const void* data, uint64_t size);
		void writeCode(const void* data, uint64_t size, uint64_t offset);
		template <typename T>
		void pushCode(const T& data);
		template <typename T>
		void writeCode(const T& data, uint64_t offset);
	private:
		uint64_t currCodeOffset() const;
		BC_MemAddress currCodeAddr() const;
	private:
		const AsmTokenListRef m_pTokenList;
		MemoryRef m_staticStack;
		CompilerError m_lastErr;
		ModuleInfoRef m_pModInfo;
		uint64_t m_nextTokenToCompile = 0;
	};

	bool isNegativeString(const std::string& value);
	std::string positiveString(const std::string& value);

	template <typename T>
	void Compiler::pushCode(const T& data)
	{
		m_pModInfo->codeMemory->push(data);
	}

	template <typename T>
	void Compiler::writeCode(const T& data, uint64_t offset)
	{
		m_pModInfo->codeMemory->write(data, offset);
	}
}