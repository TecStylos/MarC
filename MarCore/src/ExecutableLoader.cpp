#include "ExecutableLoader.h"

#include <fstream>

#include "exceptions/MarCoreException.h"

namespace MarC
{
	MarC::ExecutableInfoRef ExecutableLoader::load(const std::string& exePath)
	{
		std::ifstream iStream(exePath, std::ios::binary | std::ios::in);
		if (!iStream.is_open())
			throw MarCoreException("Unable to open executable file!");

		auto exeInfo = ExecutableInfo::create();

		deserialize(*exeInfo, iStream);

		if (!iStream.good())
			throw MarCoreException("An error occured while reading the executable file!");

		return exeInfo;
	}
}