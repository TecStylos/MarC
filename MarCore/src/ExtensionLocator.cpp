#include "ExtensionLocator.h"

namespace MarC
{
	std::map<std::string, std::set<std::string>> locateExtensions(const std::set<std::string>& baseDirs, const std::set<std::string>& extNames)
	{
		std::map<std::string, std::set<std::string>> locatedExtensions;

		for (auto& extName : extNames)
			locatedExtensions.insert({ extName, std::set<std::string>() });

		for (auto& baseDir : baseDirs)
		{
			for (auto& entry : std::filesystem::recursive_directory_iterator(baseDir))
			{
				if (!entry.is_regular_file())
					continue;
				if (entry.path().extension().string() != ".dll")
					continue;

				auto stem = entry.path().stem().string();

				auto extMatch = extNames.find(stem);
				if (extMatch == extNames.end())
					continue;

				auto& list = locatedExtensions.find(stem)->second;
				list.insert(std::filesystem::absolute(entry).string());
			}
		}

		return locatedExtensions;
	}
}
