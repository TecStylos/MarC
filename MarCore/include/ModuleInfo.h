#pragma once

#include <iostream>

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

	std::ostream& operator<<(std::ostream& outStream, const ModuleInfo& mInfo);
	std::istream& operator>>(std::istream& inStream, ModuleInfo& mInfo);
}