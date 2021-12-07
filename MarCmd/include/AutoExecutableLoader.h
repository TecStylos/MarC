#pragma once

#include <MarCore.h>

namespace MarCmd
{
	MarC::ExecutableInfoRef autoLoadExecutable(const std::string& inFile, const std::set<std::string>& modDirs);
}