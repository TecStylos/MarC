#pragma once

#include "ExecutableInfo.h"

#include <set>

namespace MarC
{
	class LinkerError
	{
	public:
		enum class Code
		{
			Success = 0,
			ModuleAlreadyExisting,
			MissingModules,
			SymbolAlreadyDefined,
			UnresolvedSymbols,
			ModuleNotFound,
			AmbigiousModule,
			CallbackError,
		};
	public:
		LinkerError() = default;
		LinkerError(Code code, const std::string& errText)
			: m_code(code), m_errText(errText)
		{}
	public:
		explicit operator bool() const;
		const std::string& getText() const;
		std::string getMessage() const;
	private:
		Code m_code = Code::Success;
		std::string m_errText = "Success!";
	};

	typedef LinkerError::Code LinkErrCode;

	class Linker;

	typedef void (*AddModuleCallback)(Linker&, const std::string&, const std::string&, void*);

	class Linker
	{
	public:
		Linker() = delete;
		Linker(std::set<std::string> modDirs);
	public:
		bool addModule(ModuleInfoRef pModInfo);
	public:
		bool update();
		bool link();
	public:
		ExecutableInfoRef getExeInfo();
	public:
		bool hasModule(const std::string& name) const;
		bool hasMissingModules() const;
		const std::set<std::string>& getMissingModules() const;
	public:
		bool autoAddMissingModules(AddModuleCallback amc, void* pPaam);
	private:
		void update(ModuleInfoRef pModInfo);
		void copySymbols(ModuleInfoRef pModInfo);
		void copyUnresolvedSymbols(ModuleInfoRef pModInfo);
		void copyReqMods(ModuleInfoRef pModInfo);
		void copyPerms(ModuleInfoRef pModInfo);
		void resolveUnresolvedSymbols();
		void resolveUnresolvedSymbolRefs();
	private:
		bool symbolNameExists(const std::string& name);
	private:
		std::string misModListStr() const;
		std::string unresSymRefsListStr() const;
	public:
		const LinkerError& lastError() const;
		void resetError();
	private:
		LinkerError m_lastErr;
		ExecutableInfoRef m_pExeInfo;
		std::set<std::string> m_missingModules;
		std::set<std::string> m_modDirs;
	};
}