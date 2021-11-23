#pragma once

#include <iostream>

#include "Serializer.h"
#include "AssemblerTypes.h"

namespace MarC
{
	class ModuleInfo;

	typedef std::shared_ptr<ModuleInfo> ModuleInfoRef;

	class ModuleInfo
	{
	public:
		std::string moduleName;
		bool extensionRequired;
		std::vector<std::string> requiredModules;
		std::vector<std::string> mandatoryPermissions;
		std::vector<std::string> optionalPermissions;
		std::vector<Symbol> definedSymbols;
		std::vector<SymbolRef> unresolvedSymbolRefs;
		MemoryRef codeMemory;
		MemoryRef staticStack;
		bool extensionLoaded;
	public:
		ModuleInfo();
	public:
		void backup();
		void recover();
	private:
		struct BackupData
		{
			uint64_t requiredModulesSize = 0;
			uint64_t mandatoryPermissionsSize = 0;
			uint64_t optionalPermissionsSize = 0;
			bool extensionRequired = false;
			bool extensionLoaded = false;
			uint64_t codeMemorySize = 0;
			uint64_t staticStackSize = 0;
			uint64_t definedSymbolsSize = 0;
			uint64_t unresolvedSymbolRefsSize = 0;
		} bud;
	};

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

	MARC_SERIALIZER_ENABLE_FIXED(ModInfoHeader);

	template<>
	inline void serialize(const ModuleInfo& modInfo, std::ostream& oStream)
	{
		ModInfoHeader header;
		header.extRequired = modInfo.extensionRequired;
		header.nReqMods = modInfo.requiredModules.size();
		header.nManPerms = modInfo.mandatoryPermissions.size();
		header.nOptPerms = modInfo.optionalPermissions.size();
		header.nDefSymbols = modInfo.definedSymbols.size();
		header.nUnresSymbolRefs = modInfo.unresolvedSymbolRefs.size();
		header.codeMemSize = modInfo.codeMemory->size();
		header.staticStackSize = modInfo.staticStack->size();

		serialize(header, oStream);
		serialize(modInfo.moduleName, oStream);

		for (auto& reqMod : modInfo.requiredModules)
			serialize(reqMod, oStream);

		for (auto& perm : modInfo.mandatoryPermissions)
			serialize(perm, oStream);

		for (auto& perm : modInfo.optionalPermissions)
			serialize(perm, oStream);

		for (auto& symbol : modInfo.definedSymbols)
			serialize(symbol, oStream);

		for (auto& symRef : modInfo.unresolvedSymbolRefs)
			serialize(symRef, oStream);

		serialize(*modInfo.codeMemory, oStream);
		serialize(*modInfo.staticStack, oStream);
	}

	template <>
	inline void deserialize(ModuleInfo& modInfo, std::istream& iStream)
	{
		modInfo = ModuleInfo();
		// TODO: Implement deserialization
	}
}