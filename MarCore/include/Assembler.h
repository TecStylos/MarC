#pragma once

#include <string>
#include "Memory.h"
#include "ModuleInfo.h"
#include "AsmInstructions.h"
#include "AsmTokenizerTypes.h"

namespace MarC
{
	class AssemblerError
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
			InvalidScope,
		};
	public:
		AssemblerError() = default;
		AssemblerError(Code code, const AsmToken& errToken, const std::string& errText, uint64_t sysErrLine, const std::string& sysErrFile)
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

	typedef AssemblerError::Code AsmErrCode;

	#define ASSEMBLER_THROW_ERROR(errCode, errText) throw AssemblerError(errCode, currToken(), errText, __LINE__, __FILE__)
	#define ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(expectedType, token) \
	ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Expected token of type '" + AsmTokenTypeToString(expectedType) + "'! Got '" + AsmTokenTypeToString(token.type) + "' with value '" + token.value + "'!")
	
	class Assembler
	{
	public:
		Assembler(const AsmTokenListRef tokenList, const std::string& moduleName = "<unnamed>");
	public:
		bool assemble();
	public:
		ModuleInfoRef getModuleInfo();
	public:
		const AssemblerError& lastError() const;
		void resetError();
	public:
		void backup();
		void recover();
	private:
		void assembleStatement();
		void assembleStatement(const std::string& statement);
		void assembleInstruction();
	private:
		void assembleSpecializedInstruction(BC_OpCodeEx& ocx);
		void assembleSpecCall(BC_OpCodeEx& ocx);
		void assembleSpecCallExtern(BC_OpCodeEx& ocx);
	private:
		void assembleArgument(BC_OpCodeEx& ocx, const InsArgument& arg);
		void assembleArgAddress(BC_OpCodeEx& ocx, const InsArgument& arg);
		void assembleArgValue(BC_OpCodeEx& ocx, const InsArgument& arg);
		void generateTypeCell(TypeCell& tc, bool& getsDereferenced);
		void generateTypeCellRegister(TypeCell& tc);
		void generateTypeCellFPRelative(TypeCell& tc);
		void generateTypeCellDTSize(TypeCell& tc);
		void generateTypeCellName(TypeCell& tc);
		void generateTypeCellString(TypeCell& tc);
		void generateTypeCellFloat(TypeCell& tc, bool getsDereferenced);
		void generateTypeCellInteger(TypeCell& tc, bool getsDereferenced);
		void assembleArgDatatype(BC_OpCodeEx& ocx, const InsArgument& arg);
	private:
		void assembleDirective();
		void assembleDirLabel();
		void assembleDirAlias();
		void assembleDirStatic();
		void assembleDirRequestModule();
		void assembleDirScope();
		void assembleDirEnd();
		void assembleDirFunction();
		void assembleDirFunctionExtern();
		void assembleDirLocal();
		void assembleDirMandatoryPermission();
		void assembleDirOptionalPermission();
	private:
		void removeNecessaryColon();
	private:
		std::string getArgAsString();
	private:
		void addSymbol(Symbol symbol);
		void addScope(const std::string& name);
		void addFuncScope(const std::string& name);
		void removeScope();
		std::string getScopedName(const std::string& name);
		void addUnresolvedSymbol(UnresolvedSymbol unresSymbol);
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
		AssemblerError m_lastErr;
		ModuleInfoRef m_pModInfo;
		std::vector<ScopeDesc> m_scopeList;
		uint64_t m_nextTokenToCompile = 0;
		uint64_t m_backupNextTokenToCompile = 0;
	private:
		friend class VirtualAsmTokenList;
	private:
		template <typename T>
		class DelayedPush
		{
		public:
			DelayedPush(Assembler& assembler)
				: DelayedPush(assembler, new T(), true)
			{}
			DelayedPush(Assembler& assembler, T& obj)
				: DelayedPush(assembler, &obj, false)
			{}
			~DelayedPush()
			{
				m_asm.writeCode(*m_pObj, m_codeOffset);
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
			DelayedPush(Assembler& assembler, T* pObj, bool destroyObjOnDestruct)
				: m_asm(assembler), m_pObj(pObj), m_destroyObjOnDestruct(destroyObjOnDestruct)
			{
				m_codeOffset = assembler.currCodeOffset();
				m_asm.pushCode(*m_pObj);
			}
			DelayedPush() = delete;
			DelayedPush(const DelayedPush&) = delete;
			DelayedPush(DelayedPush&&) = delete;
		private:
			Assembler& m_asm;
			T* m_pObj = nullptr;
			uint64_t m_codeOffset = -1;
			bool m_destroyObjOnDestruct = false;
		};
	};

	bool isNegativeString(const std::string& value);
	std::string positiveString(const std::string& value);

	template <typename T>
	void Assembler::pushCode(const T& data)
	{
		m_pModInfo->codeMemory->push(data);
	}

	template <typename T>
	void Assembler::writeCode(const T& data, uint64_t offset)
	{
		m_pModInfo->codeMemory->write(data, offset);
	}
}