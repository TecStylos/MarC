#pragma once

#include <set>
#include <iostream>

#include "ModuleInfo.h"

namespace MarC
{
	struct ExecutableInfo;
	typedef std::shared_ptr<ExecutableInfo> ExecutableInfoRef;

	struct ExecutableInfo
	{
		std::set<Symbol> symbols;
		std::set<std::string> mandatoryPermissions;
		std::set<std::string> optionalPermissions;
		std::vector<ModuleInfoRef> modules;
		std::map<std::string, uint64_t> moduleNameMap;
	public:
		static ExecutableInfoRef create();
	};

	struct ExeInfoHeader
	{
		uint64_t nSymbols;
		uint64_t nModules;
	};
	MARC_SERIALIZER_ENABLE_FIXED(ExeInfoHeader);

	template<>
	inline void serialize(const ExecutableInfo& exeInfo, std::ostream& oStream)
	{
		ExeInfoHeader header;
		header.nSymbols = exeInfo.symbols.size();
		header.nModules = exeInfo.modules.size();

		serialize(header, oStream);

		for (auto& symbol : exeInfo.symbols)
			serialize(symbol, oStream);

		serialize(exeInfo.mandatoryPermissions, oStream);
		serialize(exeInfo.optionalPermissions, oStream);

		for (auto& mod : exeInfo.modules)
			serialize(*mod, oStream);
	}

	template <>
	inline void deserialize(ExecutableInfo& exeInfo, std::istream& iStream)
	{
		exeInfo = ExecutableInfo();
		ExeInfoHeader header;
		deserialize(header, iStream);

		for (uint64_t i = 0; i < header.nSymbols; ++i)
		{
			std::string symbol;
			deserialize(symbol, iStream);
			exeInfo.symbols.insert(symbol);
		}

		deserialize(exeInfo.mandatoryPermissions, iStream);
		deserialize(exeInfo.optionalPermissions, iStream);

		for (uint64_t i = 0; i < header.nModules; ++i)
		{
			auto mod = ModuleInfo::create();
			deserialize(*mod, iStream);
			exeInfo.modules.push_back(mod);
			exeInfo.moduleNameMap.insert({ mod->moduleName, i });
		}
	}
}