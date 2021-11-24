#include "MarCmdBuilder.h"

#include <filesystem>

namespace MarCmd
{
	int Builder::build(const Settings& settings)
	{
		auto extension = std::filesystem::path(settings.inFile).extension().string();
		


		if (extension == ".mcc")
			return -1;
		if (extension == ".mca")
			return -1;
	}
}