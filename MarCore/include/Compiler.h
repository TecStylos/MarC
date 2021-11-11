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
		explicit operator bool() const;
		const std::string& getText() const;
		std::string getMessage() const;
	private:
		Code m_code = Code::Success;
		uint16_t m_line = 0;
		uint16_t m_column = 0;
		std::string m_errText = "Success!";
		uint64_t m_sysErrLine = 0;
		std::string m_sysErrFile = "<unspecified>";
	};

	typedef CompilerError::Code CompErrCode;

	#define COMPILER_THROW_ERROR(errCode, errText) throw CompilerError(errCode, currToken(), errText, __LINE__, __FILE__)
	#define COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(expectedType, token) \
	COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Expected token of type '" + AsmTokenTypeToString(expectedType) + "'! Got '" + AsmTokenTypeToString(token.type) + "' with value '" + token.value + "'!")
	
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
		void resetError();
	public:
		void backup();
		void recover();
	private:
		void compileStatement();
		void compileStatement(const std::string& statement);
		void compileInstruction();
	private:
		void compileSpecializedInstruction(BC_OpCodeEx& ocx);
		void compileSpecCall(BC_OpCodeEx& ocx);
		void compileSpecCallExtern(BC_OpCodeEx& ocx);
	private:
		void compileArgument(BC_OpCodeEx& ocx, const InsArgument& arg);
		void compileArgAddress(BC_OpCodeEx& ocx, const InsArgument& arg);
		void compileArgValue(BC_OpCodeEx& ocx, const InsArgument& arg);
		void generateTypeCell(TypeCell& tc, bool& getsDereferenced);
		void generateTypeCellRegister(TypeCell& tc);
		void generateTypeCellFPRelative(TypeCell& tc);
		void generateTypeCellDTSize(TypeCell& tc);
		void generateTypeCellName(TypeCell& tc);
		void generateTypeCellString(TypeCell& tc);
		void generateTypeCellFloat(TypeCell& tc, bool getsDereferenced);
		void generateTypeCellInteger(TypeCell& tc, bool getsDereferenced);
		void compileArgDatatype(BC_OpCodeEx& ocx, const InsArgument& arg);
	private:
		void compileDirective();
		void compileDirLabel();
		void compileDirAlias();
		void compileDirStatic();
		void compileDirRequestModule();
		void compileDirScope();
		void compileDirEnd();
		void compileDirFunction();
		void compileDirFunctionExtern();
	private:
		void removeNecessaryColon();
	private:
		std::string getArgAsString();
	private:
		void addSymbol(Symbol symbol);
		void addScope(const std::string& name);
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
		uint64_t currStaticStackOffset() const;
		BC_MemAddress currStaticStackAddr() const;
	private:
		AsmTokenListRef m_pTokenList;
		CompilerError m_lastErr;
		ModuleInfoRef m_pModInfo;
		std::vector<std::string> m_scopeList;
		uint64_t m_nextTokenToCompile = 0;
		uint64_t m_backupNextTokenToCompile = 0;
	private:
		friend class VirtualAsmTokenList;
	private:
		template <typename T>
		class DelayedPush
		{
		public:
			DelayedPush(Compiler& compiler)
				: DelayedPush(compiler, new T(), true)
			{}
			DelayedPush(Compiler& compiler, T& obj)
				: DelayedPush(compiler, &obj, false)
			{}
			~DelayedPush()
			{
				m_comp.writeCode(*m_pObj, m_codeOffset);
				if (m_destroyObjOnDestruct && m_pObj)
					delete m_pObj;
			}
			T* operator->()
			{
				return m_pObj;
			}
			T& operator*()
			{
				return *m_pObj;
			}
		private:
			DelayedPush(Compiler& compiler, T* pObj, bool destroyObjOnDestruct)
				: m_comp(compiler), m_pObj(pObj), m_destroyObjOnDestruct(destroyObjOnDestruct)
			{
				m_codeOffset = compiler.currCodeOffset();
				m_comp.pushCode(*m_pObj);
			}
			DelayedPush() = delete;
			DelayedPush(const DelayedPush&) = delete;
			DelayedPush(DelayedPush&&) = delete;
		private:
			Compiler& m_comp;
			T* m_pObj = nullptr;
			uint64_t m_codeOffset = -1;
			bool m_destroyObjOnDestruct = false;
		};
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