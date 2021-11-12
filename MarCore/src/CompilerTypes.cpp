#include "CompilerTypes.h"

namespace MarC
{
	DirectiveID DirectiveIDFromString(const std::string& value)
	{
		if (value == "") return DirectiveID::None;
		if (value == "label") return DirectiveID::Label;
		if (value == "alias") return DirectiveID::Alias;
		if (value == "static") return DirectiveID::Static;
		if (value == "reqmod") return DirectiveID::RequestModule;
		if (value == "scope") return DirectiveID::Scope;
		if (value == "end") return DirectiveID::End;
		if (value == "func") return DirectiveID::Function;
		if (value == "funx") return DirectiveID::FunctionExtern;
		if (value == "local") return DirectiveID::Local;

		return DirectiveID::Unknown;
	}

	ModuleInfo::ModuleInfo()
	{
		moduleName = "<unnamed>";
		extensionRequired = false;
		extensionLoaded = false;
		codeMemory = std::make_shared<Memory>();
		staticStack = std::make_shared<Memory>();
	}

	void ModuleInfo::backup()
	{
		bud.requiredModulesSize = requiredModules.size();
		bud.extensionRequired = extensionRequired;
		bud.extensionLoaded = extensionLoaded;
		bud.codeMemorySize = codeMemory->size();
		bud.staticStackSize = staticStack->size();
		bud.definedSymbolsSize = definedSymbols.size();
		bud.unresolvedSymbolRefsSize = unresolvedSymbolRefs.size();
	}

	void ModuleInfo::recover()
	{
		requiredModules.resize(bud.requiredModulesSize);
		extensionRequired = bud.extensionRequired;
		extensionLoaded = bud.extensionLoaded;
		codeMemory->resize(bud.codeMemorySize);
		staticStack->resize(bud.staticStackSize);
		definedSymbols.resize(bud.definedSymbolsSize);
		unresolvedSymbolRefs.resize(bud.unresolvedSymbolRefsSize);
	}
}