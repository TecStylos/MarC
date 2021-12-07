#pragma once

#include "exceptions/MarCoreException.h"

namespace MarC
{
	std::string modNameFromPath(const std::string& filepath);

	std::string modExtFromPath(const std::string& filepath);

	std::string readCodeFile(const std::string& filepath);
}