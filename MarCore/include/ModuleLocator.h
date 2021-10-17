#pragma once

#include <filesystem>
#include <string>

namespace MarC
{
	std::string locateModule(const std::string& baseDir, const std::string& modName);
}