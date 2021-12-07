#pragma once

#include <map>
#include <set>
#include <string>
#include <memory>

#include "AsmTokenizerTypes.h"

namespace MarC
{
	typedef std::shared_ptr<struct ModulePack> ModulePackRef;
	struct ModulePack
	{
		std::string name = "<unnamed>";
		AsmTokenListRef tokenList;
		std::map<std::string, AsmTokenListRef> dependencies;
	public:
		static ModulePackRef create(const std::string& name = "<unnamed>");
	};
}