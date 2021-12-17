#pragma once

#include <filesystem>
#include <string>
#include <map>
#include <set>

namespace MarC
{
	std::map<std::string, std::set<std::string>> locateModules(const std::set<std::string>& baseDirs, const std::set<std::string>& modNames);
}
