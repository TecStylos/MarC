#pragma once

#include <map>
#include <vector>
#include <string>

#include "Memory.h"
#include "BytecodeTypes.h"

namespace MarC
{
	enum class DirectiveID
	{
		None = 0,
		Unknown,
		Label,
		Alias,
		Static,
		RequestModule,
		Extension,
		Scope,
		End,
		Function,
		FunctionExtern,
		Local,
		MandatoryPermission,
		OptionalPermission,
		Macro,
		PragmaPush,
		PragmaPop,
		PragmaReplace,
	};

	struct ScopeDesc
	{
		std::string name = "<undefined>";
		bool isFuncScope = false;
		uint64_t paramSize = 0;
		uint64_t localSize = 0;
	public:
		ScopeDesc() = default;
		ScopeDesc(const std::string& name)
			: name(name)
		{}
		ScopeDesc(const std::string& name, uint64_t paramSize)
			: name(name), isFuncScope(true), paramSize(paramSize)
		{}
	};

	DirectiveID DirectiveIDFromString(const std::string& value);

	struct TypeCell
	{
		TypeCell() = default;
		TypeCell(BC_Datatype dt, BC_MemCell mc) : datatype(dt), cell(mc) {}
	public:
		BC_Datatype datatype = BC_DT_NONE;
		BC_MemCell cell = { };
	};

	enum class SymbolUsage
	{
		Value,
		Address,
	};
	MARC_SERIALIZER_ENABLE_FIXED(SymbolUsage);

	struct Symbol
	{
		std::string name = "<unnamed>";
		SymbolUsage usage = SymbolUsage::Value;
		BC_MemCell value;
	public:
		Symbol() = default;
		Symbol(const Symbol& other) = default;
		Symbol(const std::string& name) : Symbol() { this->name = name; }
		Symbol(const std::string& name, SymbolUsage usage, BC_MemCell value) : name(name), usage(usage), value(value) {}
	public:
		bool operator<(const Symbol& other) const { return name < other.name; }
	};

	struct SymbolRef
	{
		SymbolRef() = default;
		SymbolRef(const std::string& name, uint64_t offset, BC_Datatype datatype)
			: name(name), offset(offset), datatype(datatype)
		{}
	public:
		std::string name = "<unnamed>";
		uint64_t offset = -1;
		BC_Datatype datatype = BC_DT_NONE;
	};

	struct SymbolAlias
	{
		std::string name = "<unnamed>";
		std::string refName = "<unknown>";
	public:
		SymbolAlias() = default;
		SymbolAlias(const std::string& name, const std::string& refName) : name(name), refName(refName) {}
	public:
		bool operator<(const SymbolAlias& other) const { return name < other.name; }
	};

	template <>
	inline void serialize(const Symbol& symbol, std::ostream& oStream)
	{
		serialize(symbol.name, oStream);
		serialize(symbol.usage, oStream);
		serialize(symbol.value, oStream);
	}
	template <>
	inline void deserialize(Symbol& symbol, std::istream& iStream)
	{
		deserialize(symbol.name, iStream);
		deserialize(symbol.usage, iStream);
		deserialize(symbol.value, iStream);
	}
}
