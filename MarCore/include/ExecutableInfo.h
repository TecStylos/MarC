#pragma once

#include <set>
#include <iostream>

#include "AssemblerTypes.h"
#include "Serializer.h"

namespace MarC
{
	struct ExecutableInfo;
	typedef std::shared_ptr<ExecutableInfo> ExecutableInfoRef;

	struct ExecutableInfo
	{
		std::string name;
		MemoryRef codeMemory;
		MemoryRef staticStack;
		std::set<Symbol> symbols;
		std::set<std::string> mandatoryPermissions;
		std::set<std::string> optionalPermissions;
		std::set<std::string> requiredExtensions;
	public:
		static ExecutableInfoRef create();
	};

	struct ExeInfoHeader
	{
		bool hasDebugInfo;
		uint64_t nSymbols;
		uint64_t nUnresolvedSymbols;
		uint64_t nModules;
	};
	MARC_SERIALIZER_ENABLE_FIXED(ExeInfoHeader);

	template<>
	inline void serialize(const ExecutableInfo& exeInfo, std::ostream& oStream)
	{
		ExeInfoHeader header;
		//header.nModules = exeInfo.modules.size();
		//header.hasDebugInfo = exeInfo.hasDebugInfo;
		header.nSymbols = exeInfo.symbols.size();
		//header.nUnresolvedSymbols = exeInfo.unresolvedSymbols.size();

		serialize(header, oStream);

		serialize(exeInfo.mandatoryPermissions, oStream);
		serialize(exeInfo.optionalPermissions, oStream);

		//for (auto& mod : exeInfo.modules)
		//	serialize(*mod, oStream);

		if (header.hasDebugInfo)
		{
			for (auto& symbol : exeInfo.symbols)
				serialize(symbol, oStream);

			//for (auto& unresSym : exeInfo.unresolvedSymbols)
			//	serialize(unresSym, oStream);
		}
	}

	template <>
	inline void deserialize(ExecutableInfo& exeInfo, std::istream& iStream)
	{
		exeInfo = ExecutableInfo();
		ExeInfoHeader header;
		deserialize(header, iStream);

		deserialize(exeInfo.mandatoryPermissions, iStream);
		deserialize(exeInfo.optionalPermissions, iStream);

		for (uint64_t i = 0; i < header.nModules; ++i)
		{
			//auto mod = ModuleInfo::create();
			//deserialize(*mod, iStream);
			//exeInfo.modules.push_back(mod);
			//exeInfo.moduleNameMap.insert({ mod->moduleName, i });
		}

		if (header.hasDebugInfo)
		{
			for (uint64_t i = 0; i < header.nSymbols; ++i)
			{
				Symbol symbol;
				deserialize(symbol, iStream);
				//exeInfo.symbols.insert(symbol);
			}

			for (uint64_t i = 0; i < header.nUnresolvedSymbols; ++i)
			{
				UnresolvedSymbol unresSym;
				deserialize(unresSym, iStream);
				//exeInfo.unresolvedSymbols.insert(unresSym);
			}
		}
	}
}