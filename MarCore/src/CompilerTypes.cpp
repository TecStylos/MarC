#include "CompilerTypes.h"

namespace MarC
{
	DirectiveID DirectiveIDFromString(const std::string& value)
	{
		if (value == "") return DirectiveID::None;
		if (value == "label") return DirectiveID::Label;
		if (value == "alias") return DirectiveID::Alias;
		if (value == "static") return DirectiveID::Static;

		return DirectiveID::Unknown;
	}

	ModuleInfo::ModuleInfo()
	{
		moduleName = "<unnamed>";
		codeMemory = std::make_shared<Memory>();
	}

	void ModuleInfo::backup()
	{
		bud.requiredModulesSize = requiredModules.size();
		bud.codeMemorySize = codeMemory->size();
	}

	void ModuleInfo::recover()
	{
		requiredModules.resize(bud.requiredModulesSize);
		codeMemory->resize(bud.codeMemorySize);
	}
}