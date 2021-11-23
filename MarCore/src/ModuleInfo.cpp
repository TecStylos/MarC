#include "ModuleInfo.h"

namespace MarC
{
	struct ModInfoHeader
	{
		bool extRequired;
		uint64_t nReqMods;
		uint64_t nManPerms;
		uint64_t nOptPerms;
		uint64_t nDefSymbols;
		uint64_t nUnresSymbolRefs;
		uint64_t codeMemSize;
		uint64_t staticStackSize;
	};

	ModuleInfo::ModuleInfo()
	{
		moduleName = "<unnamed>";
		extensionRequired = false;
		extensionLoaded = false;
		codeMemory = std::make_shared<Memory>();
		staticStack = std::make_shared<Memory>();
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
		unresolvedSymbolRefs.resize(bud.unresolvedSymbolRefsSize);
	}

	std::ostream& operator<<(std::ostream& outStream, const ModuleInfo& mInfo)
	{
		ModInfoHeader header;
		header.extRequired = mInfo.extensionRequired;
		header.nReqMods = mInfo.requiredModules.size();
		header.nManPerms = mInfo.mandatoryPermissions.size();
		header.nOptPerms = mInfo.optionalPermissions.size();
		header.nDefSymbols = mInfo.definedSymbols.size();
		header.nUnresSymbolRefs = mInfo.unresolvedSymbolRefs.size();
		header.codeMemSize = mInfo.codeMemory->size();
		header.staticStackSize = mInfo.staticStack->size();

		outStream.write((const char*)&header, sizeof(header));
		outStream.write(mInfo.moduleName.c_str(), mInfo.moduleName.size() + 1);

		for (auto& reqMod : mInfo.requiredModules)
			outStream.write(reqMod.c_str(), reqMod.size() + 1);

		for (auto& perm : mInfo.mandatoryPermissions)
			outStream.write(perm.c_str(), perm.size() + 1);

		for (auto& perm : mInfo.optionalPermissions)
			outStream.write(perm.c_str(), perm.size() + 1);

		for (auto& symbol : mInfo.definedSymbols)
		{
			outStream.write(symbol.name.c_str(), symbol.name.size() + 1);
			outStream.write((const char*)&symbol.usage, sizeof(symbol.usage));
			outStream.write((const char*)&symbol.value, sizeof(symbol.value));
		}

		for (auto& symRef : mInfo.unresolvedSymbolRefs)
		{
			outStream.write((const char*)&symRef.datatype, sizeof(symRef.datatype));
			outStream.write(symRef.name.c_str(), symRef.name.size() + 1);
			outStream.write((const char*)&symRef.offset, sizeof(symRef.offset));
		}

		outStream.write((const char*)mInfo.codeMemory->getBaseAddress(), mInfo.codeMemory->size());

		outStream.write((const char*)mInfo.staticStack->getBaseAddress(), mInfo.staticStack->size());

		return outStream;
	}
	std::istream& operator>>(std::istream& inStream, ModuleInfo& mInfo)
	{
		mInfo = ModuleInfo();

		return inStream;
	}
}