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
	};

	struct ExeInfoHeader
	{
		uint64_t nSymbols;
		uint64_t nManPerms;
		uint64_t nOptPerms;
		uint64_t nModules;
	};
	MARC_SERIALIZER_ENABLE_FIXED(ExeInfoHeader);

	template<>
	inline void serialize(const ExecutableInfo& exeInfo, std::ostream& oStream)
	{
		ExeInfoHeader header;
		header.nSymbols = exeInfo.symbols.size();
		header.nManPerms = exeInfo.mandatoryPermissions.size();
		header.nOptPerms = exeInfo.optionalPermissions.size();
		header.nModules = exeInfo.modules.size();

		serialize(header, oStream);

		for (auto& symbol : exeInfo.symbols)
			serialize(symbol, oStream);

		for (auto& perm : exeInfo.mandatoryPermissions)
			serialize(perm, oStream);

		for (auto& perm : exeInfo.optionalPermissions)
			serialize(perm, oStream);

		for (auto& mod : exeInfo.modules)
			serialize(*mod, oStream);
	}

	template <>
	inline void deserialize(ExecutableInfo& exeInfo, std::istream& iStream)
	{
		exeInfo = ExecutableInfo();
		// TODO: Implement deserialization
	}
}