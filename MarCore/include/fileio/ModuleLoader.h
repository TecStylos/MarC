#pragma once

#include "ModulePack.h"

namespace MarC
{
	class ModuleLoader
	{
	public:
		static ModulePackRef load(const std::string& modPath, const std::set<std::string>& modDirs);

		static void loadDependencies(AsmTokenListRef tokenList, std::map<std::string, AsmTokenListRef>& dependencies, const std::set<std::string>& modDirs);
	};
}