#include "fileio/ModuleLocator.h"

namespace MarC
{
	std::map<std::string, std::set<std::string>> locateModules(const std::set<std::string>& baseDirs, const std::set<std::string>& modNames)
	{
		std::map<std::string, std::set<std::string>> locatedModules;

		for (auto& modName : modNames)
			locatedModules.insert({ modName, std::set<std::string>() });

		for (auto& baseDir : baseDirs)
		{
			for (auto& entry : std::filesystem::recursive_directory_iterator(baseDir))
			{
				if (!entry.is_regular_file())
					continue;
				if (entry.path().extension().string() != ".mca")
					continue;

				auto stem = entry.path().stem().string();

				auto modMatch = modNames.find(stem);
				if (modMatch == modNames.end())
					continue;

				auto& list = locatedModules.find(stem)->second;
				list.insert(std::filesystem::absolute(entry).string());
			}
		}

		return locatedModules;
	}
}
