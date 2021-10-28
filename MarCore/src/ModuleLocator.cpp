#include "ModuleLocator.h"

namespace MarC
{
	std::map<std::string, std::vector<std::string>> locateModules(const std::set<std::string>& baseDirs, const std::set<std::string>& modNames)
	{
		std::map<std::string, std::vector<std::string>> locatedModules;

		for (auto& modName : modNames)
			locatedModules.insert({ modName, std::vector<std::string>() });

		for (auto& baseDir : baseDirs)
		{
			for (auto& p : std::filesystem::recursive_directory_iterator(baseDir))
			{
				if (!p.is_regular_file())
					continue;
				if (p.path().extension().string() != ".mca")
					continue;

				auto stem = p.path().stem().string();

				auto modMatch = modNames.find(stem);
				if (modMatch == modNames.end())
					continue;

				auto& list = locatedModules.find(stem)->second;
				list.push_back(p.path().string());
			}
		}

		return locatedModules;
	}
}
