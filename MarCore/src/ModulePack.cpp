#include "ModulePack.h"

namespace MarC
{
	ModulePackRef ModulePack::create(const std::string& name)
	{
		auto temp = std::make_shared<ModulePack>();
		temp->name = name;
		return temp;
	}
}