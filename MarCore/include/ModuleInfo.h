#pragma once

#include "ExecutableInfo.h"

namespace MarC
{
	class ModuleInfo;

	typedef std::shared_ptr<ModuleInfo> ModuleInfoRef;

	class ModuleInfo
	{
	public:
		ExecutableInfoRef exeInfo;
		std::set<UnresolvedSymbol> unresolvedSymbols;
		std::vector<SymbolRef> unresolvedSymbolRefs;
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
			bool extensionRequired = false;
			bool extensionLoaded = false;
			uint64_t requiredModulesSize = 0;
			uint64_t mandatoryPermissionsSize = 0;
			uint64_t optionalPermissionsSize = 0;
			uint64_t definedSymbolsSize = 0;
			uint64_t unresolvedSymbolsSize = 0;
			uint64_t unresolvedSymbolRefsSize = 0;
			uint64_t codeMemorySize = 0;
			uint64_t staticStackSize = 0;
		} bud;
	};

	struct ModInfoHeader
	{
		bool extRequired;
		uint64_t nDefSymbols;
		uint64_t nUnresSymbols;
		uint64_t nUnresSymbolRefs;
	};

	MARC_SERIALIZER_ENABLE_FIXED(ModInfoHeader);

	template<>
	inline void serialize(const ModuleInfo& modInfo, std::ostream& oStream)
	{
		ModInfoHeader header;
		//header.extRequired = modInfo.extensionRequired;
		//header.nDefSymbols = modInfo.definedSymbols.size();
		header.nUnresSymbols = modInfo.unresolvedSymbols.size();
		header.nUnresSymbolRefs = modInfo.unresolvedSymbolRefs.size();

		serialize(header, oStream);

		//serialize(modInfo.requiredModules, oStream);
		//serialize(modInfo.mandatoryPermissions, oStream);
		//serialize(modInfo.optionalPermissions, oStream);

		//for (auto& symbol : modInfo.definedSymbols)
		//	serialize(symbol, oStream);

		for (auto& symRef : modInfo.unresolvedSymbolRefs)
			serialize(symRef, oStream);

		for (auto& unresSym : modInfo.unresolvedSymbols)
			serialize(unresSym, oStream);

		//serialize(*modInfo.codeMemory, oStream);
		//serialize(*modInfo.staticStack, oStream);
	}

	template <>
	inline void deserialize(ModuleInfo& modInfo, std::istream& iStream)
	{
		modInfo = ModuleInfo();
		ModInfoHeader header;
		deserialize(header, iStream);

		//deserialize(modInfo.requiredModules, iStream);
		//deserialize(modInfo.mandatoryPermissions, iStream);
		//deserialize(modInfo.optionalPermissions, iStream);

		//modInfo.extensionRequired = header.extRequired;

		for (uint64_t i = 0; i < header.nDefSymbols; ++i)
		{
			Symbol symbol;
			deserialize(symbol, iStream);
			//modInfo.definedSymbols.push_back(symbol);
		}

		for (uint64_t i = 0; i < header.nUnresSymbolRefs; ++i)
		{
			SymbolRef symRef;
			deserialize(symRef, iStream);
			//modInfo.unresolvedSymbolRefs.push_back(symRef);
		}

		for (uint64_t i = 0; i < header.nUnresSymbols; ++i)
		{
			UnresolvedSymbol unresSym;
			deserialize(unresSym, iStream);
			//modInfo.unresolvedSymbols.push_back(unresSym);
		}

		//modInfo.codeMemory = Memory::create();
		//deserialize(*modInfo.codeMemory, iStream);
		//modInfo.staticStack = Memory::create();
		//deserialize(*modInfo.staticStack, iStream);
	}
}