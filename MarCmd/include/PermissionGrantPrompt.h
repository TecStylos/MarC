#pragma once

#include <set>
#include <string>

namespace MarCmd
{
	enum class PermissionPromptType
	{
		Mandatory,
		Optional
	};
	bool permissionGrantPrompt(PermissionPromptType ppt, const std::set<std::string>& toGrant, std::set<std::string>& output);
}