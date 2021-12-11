#pragma once

#include <string>
#include "Memory.h"
#include "ModuleInfo.h"
#include "ModulePack.h"
#include "AsmInstructions.h"
#include "AsmTokenizerTypes.h"
#include "errors/AssemblerError.h"

namespace MarC
{
	class Assembler
	{
	public:
		Assembler(const ModulePackRef modPack);
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
		void assembleMacroExpansion();
	private:
		void assembleSpecializedInstruction(BC_OpCodeEx& ocx);
		void assembleSpecCall(BC_OpCodeEx& ocx);
		void assembleSpecCallExtern(BC_OpCodeEx& ocx);
	private:
		void assembleArgument(BC_OpCodeEx& ocx, const InsArgument& arg);
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
		void assembleDirExtension();
		void assembleDirScope();
		void assembleDirEnd();
		void assembleDirFunction();
		void assembleDirFunctionExtern();
		void assembleDirLocal();
		void assembleDirMandatoryPermission();
		void assembleDirOptionalPermission();
		void assembleDirMacro();
		void assembleDirPragmaPush();
		void assembleDirPragmaPop();
		void assembleDirPragmaReplace();
	public:
		void assembleSubTokenList(AsmTokenListRef tokenList);
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
		void addSymbolAlias(SymbolAlias symAlias);
	private:
		bool macroExists(const std::string& macroName);
		void expandMacro(const std::string& macroName, const std::vector<AsmTokenList>& parameters);
		void addMacro(const std::string& macroName, const Macro& macro);
	private:
		bool isInstruction();
		bool isMacro();
		bool isDirective();
	private:
		const AsmToken& nextToken();
		const AsmToken& prevToken();
		const AsmToken& currToken() const;
		const AsmToken& nextTokenNoModify();
		const AsmToken& prevTokenNoModify();
		const AsmToken& currTokenNoModify() const;
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
		AsmTokenListRef m_pCurrTokenList;
		ModulePackRef m_pModPack;
		AssemblerError m_lastErr;
		ModuleInfoRef m_pModInfo;
		std::vector<ScopeDesc> m_scopeList;
		std::vector<AsmToken> m_pragmaList;
		uint64_t m_nextPragmaIndex = 0;
		std::set<std::string> m_resolvedDependencies;
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
		m_pModInfo->exeInfo->codeMemory->push(data);
	}

	template <typename T>
	void Assembler::writeCode(const T& data, uint64_t offset)
	{
		m_pModInfo->exeInfo->codeMemory->write(data, offset);
	}
}