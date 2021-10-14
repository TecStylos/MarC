#pragma once

#include <map>
#include <vector>
#include <string>

#include "Memory.h"
#include "BytecodeTypes.h"

namespace MarC
{
	struct TypeCell
	{
		TypeCell() = default;
		TypeCell(BC_Datatype dt, BC_MemCell mc) : datatype(dt), cell(mc) {}
	public:
		BC_Datatype datatype;
		BC_MemCell cell;
	};

	enum LabelUsage
	{
		LABEL_USAGE_DEFAULT,
		LABEL_USAGE_ADDRESS,
	};
	struct Label
	{
		LabelUsage usage;
		BC_MemCell value;
		Label(LabelUsage usage, BC_MemCell value) : usage(usage), value(value) {}
	};
	struct LabelRef
	{
		LabelRef(const std::string& name, uint64_t offset, BC_Datatype datatype)
			: name(name), offset(offset), datatype(datatype)
		{}
	public:
		std::string name;
		uint64_t offset;
		BC_Datatype datatype;
	};
	typedef std::vector<LabelRef> LabelRefList;
	typedef std::map<std::string, Label> LabelMap;

	class ModuleInfo
	{
	public:
		std::string moduleName;
		std::vector<std::string> requiredModules;
		MemoryRef codeMemory;
		LabelMap labels;
		LabelRefList unresolvedRefs;
		uint64_t nLinesParsed;
	public:
		ModuleInfo();
	public:
		uint64_t getErrLine() const;
	public:
		void backup();
		void recover();
	private:
		struct BackupData
		{
			uint64_t requiredModulesSize = 0;
			uint64_t codeMemorySize = 0;
			uint64_t nLinesParsed = 0;
		} bud;
	};

	typedef std::shared_ptr<ModuleInfo> ModuleInfoRef;
}