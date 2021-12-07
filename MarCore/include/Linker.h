#pragma once

#include "ModuleInfo.h"

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
		Linker(ModuleInfoRef modInfo);
	public:
		bool addModule(ModuleInfoRef pModInfo);
	public:
		bool update();
		bool link();
	public:
		ExecutableInfoRef getExeInfo();
	private:
		void resolveUnresolvedSymbols();
		void resolveUnresolvedSymbolRefs();
	private:
		bool symbolNameExists(const std::string& name);
	public:
		const LinkerError& lastError() const;
		void resetError();
	private:
		ModuleInfoRef m_modInfo;
		LinkerError m_lastErr;
	};
}