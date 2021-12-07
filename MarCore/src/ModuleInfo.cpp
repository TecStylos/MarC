#include "ModuleInfo.h"

namespace MarC
{
	ModuleInfo::ModuleInfo()
	{
		exeInfo = ExecutableInfo::create();
	}

	void ModuleInfo::backup()
	{
		bud.symAliases = symbolAliases.size();
		bud.unresSymRefSize = unresolvedSymbolRefs.size();
	}

	void ModuleInfo::recover()
	{
	}

	ModuleInfoRef ModuleInfo::create()
	{
		return std::make_shared<ModuleInfo>();
	}
}