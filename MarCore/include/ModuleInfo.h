#pragma once

#include "AssemblerTypes.h"

namespace MarC
{
	class ModuleInfo;

	typedef std::shared_ptr<ModuleInfo> ModuleInfoRef;

	class ModuleInfo
	{
	public:
		std::string moduleName;
		std::vector<std::string> requiredModules;
		std::vector<std::string> mandatoryPermissions;
		std::vector<std::string> optionalPermissions;
		bool extensionRequired;
		bool extensionLoaded;
		MemoryRef codeMemory;
		MemoryRef staticStack;
		std::vector<Symbol> definedSymbols;
		std::vector<SymbolRef> unresolvedSymbolRefs;
	public:
		ModuleInfo();
	public:
		void backup();
		void recover();
	public:
		static ModuleInfoRef loadFromFile(const std::string& path); // TODO: Implement
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
}