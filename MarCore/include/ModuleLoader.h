#pragma once

#include "Module.h"

namespace MarC
{
	class ModuleLoader
	{
	public:
		static ModuleRef load(const std::string& modPath, const std::set<std::string>& modDirs);
	private:
		static void loadDependencies(AsmTokenListRef tokenList, std::map<std::string, AsmTokenListRef>& dependencies, const std::set<std::string>& modDirs);
	};
}