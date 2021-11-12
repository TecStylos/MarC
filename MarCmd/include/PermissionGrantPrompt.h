#pragma once

#include <set>
#include <string>

namespace MarCmd
{
	bool permissionGrantPrompt(const std::set<std::string>& toGrant, std::set<std::string>& output);
}