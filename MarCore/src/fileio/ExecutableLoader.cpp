#include "fileio/ExecutableLoader.h"

#include <fstream>

#include "errors/MarCoreError.h"

namespace MarC
{
	MarC::ExecutableInfoRef ExecutableLoader::load(const std::string& exePath)
	{
		std::ifstream iStream(exePath, std::ios::binary | std::ios::in);
		if (!iStream.is_open())
			throw MarCoreError("ExeLoadError", "Unable to open the executable file!");

		auto exeInfo = ExecutableInfo::create();

		deserialize(*exeInfo, iStream);

		if (!iStream.good())
			throw MarCoreError("ExeLoadError", "An error occured while reading the executable file!");

		return exeInfo;
	}
}