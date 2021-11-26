#include "ModuleInfo.h"

namespace MarC
{
	ModuleInfo::ModuleInfo()
	{
		moduleName = "<unnamed>";
		extensionRequired = false;
		extensionLoaded = false;
		codeMemory = Memory::create();
		staticStack = Memory::create();
	}

	void ModuleInfo::backup()
	{
		bud.requiredModulesSize = requiredModules.size();
		bud.mandatoryPermissionsSize = mandatoryPermissions.size();
		bud.optionalPermissionsSize = optionalPermissions.size();
		bud.extensionRequired = extensionRequired;
		bud.extensionLoaded = extensionLoaded;
		bud.codeMemorySize = codeMemory->size();
		bud.staticStackSize = staticStack->size();
		bud.definedSymbolsSize = definedSymbols.size();
		bud.unresolvedSymbolsSize = unresolvedSymbols.size();
		bud.unresolvedSymbolRefsSize = unresolvedSymbolRefs.size();
	}

	void ModuleInfo::recover()
	{
		requiredModules.resize(bud.requiredModulesSize);
		mandatoryPermissions.resize(bud.mandatoryPermissionsSize);
		optionalPermissions.resize(bud.optionalPermissionsSize);
		extensionRequired = bud.extensionRequired;
		extensionLoaded = bud.extensionLoaded;
		codeMemory->resize(bud.codeMemorySize);
		staticStack->resize(bud.staticStackSize);
		definedSymbols.resize(bud.definedSymbolsSize);
		unresolvedSymbols.resize(bud.unresolvedSymbolsSize);
		unresolvedSymbolRefs.resize(bud.unresolvedSymbolRefsSize);
	}

	ModuleInfoRef ModuleInfo::create()
	{
		return std::make_shared<ModuleInfo>();
	}
}