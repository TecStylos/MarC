#include "ModuleLocator.h"

namespace MarC
{
	std::string locateModule(const std::string& baseDir, const std::string& modName)
	{
		std::filesystem::recursive_directory_iterator it;
		for (auto& p : std::filesystem::recursive_directory_iterator(baseDir))
		{
			if (!p.is_regular_file())
				continue;
			if (p.path().extension().string() != ".mca")
				continue;
			if (p.path().stem().string() != modName)
				continue;

			return p.path().generic_string();
		}

		return "";
	}
}