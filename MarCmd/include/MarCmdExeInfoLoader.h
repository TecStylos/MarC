#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>

#include <MarCore.h>
#include "MarCmdSettings.h"
#include "MarCmdModuleAdder.h"

namespace MarCmd
{
	MarC::ExecutableInfoRef loadExeInfo(const Settings& settings);
}