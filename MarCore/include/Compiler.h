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
			AlreadyInGlobalScope,
			InternalError,
		};
	public:
		CompilerError() = default;
		CompilerError(Code code, const AsmToken& errToken, const std::string& errText, uint64_t sysErrLine, const std::string& sysErrFile)
			: m_code(code), m_line(errToken.line), m_column(errToken.column), m_errText(errText), m_sysErrLine(sysErrLine), m_sysErrFile(sysErrFile)
		{}
	public:
		operator bool() const;
		const std::string& getText() const;
		std::string getMessage() const;
	private:
		Code m_code = Code::Success;
		uint16_t m_line;
		uint16_t m_column;
		std::string m_errText = "Success!";
		uint64_t m_sysErrLine = 0;
		std::string m_sysErrFile = "<unspecified>";
	};

	typedef CompilerError::Code CompErrCode;

	#define COMPILER_RETURN_WITH_ERROR(errCode, errText) { m_lastErr = CompilerError(errCode, currToken(), errText, __LINE__, __FILE__); return false; }
	#define COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(expectedType, token) COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Expected token of type '" + AsmTokenTypeToString(expectedType) + "'! Got '" + AsmTokenTypeToString(token.type) + "' with value '" + token.value + "'!")

	class Compiler
	{
	public:
		Compiler(const AsmTokenListRef tokenList, const std::string& moduleName = "<unnamed>");
	public:
		bool compile();
	public:
		ModuleInfoRef getModuleInfo();
	public:
		const CompilerError& lastError() const;
	private:
		bool compileStatement();
		bool compileStatement(const std::string& statement);
		bool compileInstruction();
	private:
		bool compileSpecializedInstruction(BC_OpCodeEx& ocx);
		bool compileSpecCall(BC_OpCodeEx& ocx);
	private:
		bool compileArgument(BC_OpCodeEx& ocx, const InsArgument& arg);
		bool compileArgAddress(BC_OpCodeEx& ocx, const InsArgument& arg);
		bool compileArgValue(BC_OpCodeEx& ocx, const InsArgument& arg);
		bool generateTypeCell(TypeCell& tc, bool& getsDereferenced);
		bool generateTypeCellRegister(TypeCell& tc);
		bool generateTypeCellFPRelative(TypeCell& tc);
		bool generateTypeCellName(TypeCell& tc);
		bool generateTypeCellString(TypeCell& tc);
		bool generateTypeCellFloat(TypeCell& tc, bool getsDereferenced);
		bool generateTypeCellInteger(TypeCell& tc, bool getsDereferenced);
		bool compileArgDatatype(BC_OpCodeEx& ocx, const InsArgument& arg);
	private:
		bool compileDirective();
		bool compileDirLabel();
		bool compileDirAlias();
		bool compileDirStatic();
		bool compileDirRequestModule();
		bool compileDirScope();
		bool compileDirEnd();
		bool compileDirFunction();
	private:
		bool removeNecessaryColon();
	private:
		std::string getArgAsString();
	private:
		bool addSymbol(const std::string& name, const Symbol& symbol);
		bool addScope(const std::string& name);
		std::string getScopedName(const std::string& name);
	private:
		bool isInstructionLike();
		bool isDirectiveLike();
	private:
		const AsmToken& nextToken();
		const AsmToken& prevToken();
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
		AsmTokenListRef m_pTokenList;
		CompilerError m_lastErr;
		ModuleInfoRef m_pModInfo;
		std::vector<std::string> m_scopeList;
		uint64_t m_nextTokenToCompile = 0;
	private:
		friend class VirtualAsmTokenList;
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