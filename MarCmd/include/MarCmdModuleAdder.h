#pragma once

#include <string>

#include <Linker.h>

namespace MarCmd
{
	std::string modNameFromPath(const std::string& filepath);

	void addModule(MarC::Linker& linker, const std::string& modPath, const std::string& modName, void* pParam);
}