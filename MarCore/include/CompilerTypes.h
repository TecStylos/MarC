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
	struct Symbol
	{
		SymbolUsage usage = SymbolUsage::Value;
		BC_MemCell value;
		Symbol() = default;
		Symbol(SymbolUsage usage, BC_MemCell value) : usage(usage), value(value) {}
	};
	struct SymbolRef
	{
		SymbolRef(const std::string& name, uint64_t offset, BC_Datatype datatype)
			: name(name), offset(offset), datatype(datatype)
		{}
	public:
		std::string name;
		uint64_t offset;
		BC_Datatype datatype;
	};
	typedef std::vector<SymbolRef> SymbolRefList;
	typedef std::map<std::string, Symbol> SymbolMap;

	class ModuleInfo
	{
	public:
		std::string moduleName;
		std::vector<std::string> requiredModules;
		MemoryRef codeMemory;
		SymbolMap symbols;
		SymbolRefList unresolvedRefs;
	public:
		ModuleInfo();
	public:
		void backup();
		void recover();
	private:
		struct BackupData
		{
			uint64_t requiredModulesSize = 0;
			uint64_t codeMemorySize = 0;
		} bud;
	};

	typedef std::shared_ptr<ModuleInfo> ModuleInfoRef;
}