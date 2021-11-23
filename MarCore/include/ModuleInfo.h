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
	public:
		static ModuleInfoRef create();
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
		header.nDefSymbols = modInfo.definedSymbols.size();
		header.nUnresSymbolRefs = modInfo.unresolvedSymbolRefs.size();
		header.codeMemSize = modInfo.codeMemory->size();
		header.staticStackSize = modInfo.staticStack->size();

		serialize(header, oStream);
		serialize(modInfo.moduleName, oStream);

		serialize(modInfo.requiredModules, oStream);
		serialize(modInfo.mandatoryPermissions, oStream);
		serialize(modInfo.optionalPermissions, oStream);

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
		ModInfoHeader header;
		deserialize(header, iStream);
		deserialize(modInfo.moduleName, iStream);

		deserialize(modInfo.requiredModules, iStream);
		deserialize(modInfo.mandatoryPermissions, iStream);
		deserialize(modInfo.optionalPermissions, iStream);

		for (uint64_t i = 0; i < header.nDefSymbols; ++i)
		{
			Symbol symbol;
			deserialize(symbol, iStream);
			modInfo.definedSymbols.push_back(symbol);
		}

		for (uint64_t i = 0; i < header.nUnresSymbolRefs; ++i)
		{
			SymbolRef symRef;
			deserialize(symRef, iStream);
			modInfo.unresolvedSymbolRefs.push_back(symRef);
		}

		modInfo.codeMemory = Memory::create();
		deserialize(*modInfo.codeMemory, iStream);
		modInfo.staticStack = Memory::create();
		deserialize(*modInfo.staticStack, iStream);
	}
}