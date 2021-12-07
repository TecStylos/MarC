#pragma once

#include <map>
#include <set>
#include <string>
#include <memory>

#include "AsmTokenizerTypes.h"

namespace MarC
{
	struct Module
	{
		std::string name = "<unnamed>";
		AsmTokenListRef tokenList;
		std::map<std::string, AsmTokenListRef> dependencies;
	};
	typedef std::shared_ptr<Module> ModuleRef;
}