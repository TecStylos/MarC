#include "CompilerTypes.h"

namespace MarC
{
	uint64_t ModuleInfo::getErrLine() const
	{
		return nLinesParsed + 1;
	}

	void ModuleInfo::backup()
	{
		bud.requiredModulesSize = requiredModules.size();
		bud.codeMemorySize = codeMemory->size();
		bud.nLinesParsed = nLinesParsed;
	}

	void ModuleInfo::recover()
	{
		requiredModules.resize(bud.requiredModulesSize);
		codeMemory->resize(bud.codeMemorySize);
		nLinesParsed = bud.nLinesParsed;
	}

	ModuleInfo::ModuleInfo()
	{
		moduleName = "<unnamed>";
		codeMemory = std::make_shared<Memory>();
		nLinesParsed = 0;
	}
}