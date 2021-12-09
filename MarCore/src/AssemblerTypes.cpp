#include "AssemblerTypes.h"

namespace MarC
{
	DirectiveID DirectiveIDFromString(const std::string& value)
	{
		if (value == "") return DirectiveID::None;
		if (value == "label") return DirectiveID::Label;
		if (value == "alias") return DirectiveID::Alias;
		if (value == "static") return DirectiveID::Static;
		if (value == "reqmod") return DirectiveID::RequestModule;
		if (value == "extension") return DirectiveID::Extension;
		if (value == "scope") return DirectiveID::Scope;
		if (value == "end") return DirectiveID::End;
		if (value == "func") return DirectiveID::Function;
		if (value == "funx") return DirectiveID::FunctionExtern;
		if (value == "local") return DirectiveID::Local;
		if (value == "manperm") return DirectiveID::MandatoryPermission;
		if (value == "optperm") return DirectiveID::OptionalPermission;
		if (value == "macro") return DirectiveID::Macro;

		return DirectiveID::Unknown;
	}
}