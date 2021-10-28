#pragma once

#include <vector>
#include <filesystem>
#include <string>
#include <map>
#include <set>

namespace MarC
{
	std::map<std::string, std::vector<std::string>> locateModules(const std::set<std::string>& baseDirs, const std::set<std::string>& modNames);
}
