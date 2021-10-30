#pragma once

#include "LinkerOutput.h"

#include <set>

namespace MarC
{
	class Linker
	{
	public:
		Linker();
	public:
		bool addModule(ModuleInfoRef pModInfo);
	public:
		void update();
		bool link();
	public:
		ExecutableInfoRef getExeInfo();
	public:
		bool hasModule(const std::string& name) const;
		bool hasMissingModules() const;
		const std::set<std::string>& getMissingModules() const;
	private:
		bool loadReqMods();
		void copySymbols();
		void copySymbols(ModuleInfoRef pModInfo);
		void copyReqMods();
		void copyReqMods(ModuleInfoRef pModInfo);
		bool resolveSymbols();
	private:
		ExecutableInfoRef m_pExeInfo;
		std::set<Symbol> m_symbols;
		std::set<std::string> m_missingModules;
	};
}