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
		std::string name = "<unnamed>";
		MemoryRef codeMemory;
		MemoryRef staticStack;
		std::set<std::string> mandatoryPermissions;
		std::set<std::string> optionalPermissions;
		std::set<std::string> requiredExtensions;
		std::set<Symbol> symbols;
	public:
		static ExecutableInfoRef create();
	};

	struct ExeInfoHeader
	{
		uint64_t nSymbols;
	};
	MARC_SERIALIZER_ENABLE_FIXED(ExeInfoHeader);

	template<>
	inline void serialize(const ExecutableInfo& exeInfo, std::ostream& oStream)
	{
		ExeInfoHeader header;
		header.nSymbols = exeInfo.symbols.size();

		serialize(header, oStream);

		serialize(exeInfo.name, oStream);
		serialize(*exeInfo.codeMemory, oStream);
		serialize(*exeInfo.staticStack, oStream);
		serialize(exeInfo.mandatoryPermissions, oStream);
		serialize(exeInfo.optionalPermissions, oStream);
		serialize(exeInfo.requiredExtensions, oStream);

		for (auto& symbol : exeInfo.symbols)
			serialize(symbol, oStream);
	}

	template <>
	inline void deserialize(ExecutableInfo& exeInfo, std::istream& iStream)
	{
		exeInfo = ExecutableInfo();
		exeInfo.codeMemory = Memory::create();
		exeInfo.staticStack = Memory::create();

		ExeInfoHeader header;
		deserialize(header, iStream);

		deserialize(exeInfo.name, iStream);
		deserialize(*exeInfo.codeMemory, iStream);
		deserialize(*exeInfo.staticStack, iStream);
		deserialize(exeInfo.mandatoryPermissions, iStream);
		deserialize(exeInfo.optionalPermissions, iStream);
		deserialize(exeInfo.requiredExtensions, iStream);

		for (uint64_t i = 0; i < header.nSymbols; ++i)
		{
			Symbol symbol;
			deserialize(symbol, iStream);
			exeInfo.symbols.insert(symbol);
		}

	}
}