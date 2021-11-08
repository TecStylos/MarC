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
		if (value == "string") return DirectiveID::StaticString;

		return DirectiveID::Unknown;
	}

	ModuleInfo::ModuleInfo()
	{
		moduleName = "<unnamed>";
		requiresExtension = false;
		codeMemory = std::make_shared<Memory>();
		staticStack = std::make_shared<Memory>();
	}

	void ModuleInfo::backup()
	{
		bud.requiredModulesSize = requiredModules.size();
		bud.requiresExtension = requiresExtension;
		bud.codeMemorySize = codeMemory->size();
		bud.staticStackSize = staticStack->size();
		bud.definedSymbolsSize = definedSymbols.size();
		bud.unresolvedSymbolRefsSize = unresolvedSymbolRefs.size();
	}

	void ModuleInfo::recover()
	{
		requiredModules.resize(bud.requiredModulesSize);
		requiresExtension = bud.requiresExtension;
		codeMemory->resize(bud.codeMemorySize);
		staticStack->resize(bud.staticStackSize);
		definedSymbols.resize(bud.definedSymbolsSize);
		unresolvedSymbolRefs.resize(bud.unresolvedSymbolRefsSize);
	}
}