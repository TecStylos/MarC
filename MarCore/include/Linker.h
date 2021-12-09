#pragma once

#include <set>

#include "ModuleInfo.h"
#include "errors/LinkerError.h"

namespace MarC
{
	class Linker;

	typedef void (*AddModuleCallback)(Linker&, const std::string&, const std::string&, void*);

	class Linker
	{
	public:
		Linker() = delete;
		Linker(ModuleInfoRef modInfo);
	public:
		bool link();
	public:
		ExecutableInfoRef getExeInfo();
	private:
		void resolveSymbolAliases();
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