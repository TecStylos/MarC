#include "ExtensionLocator.h"

#include "PluS/Defines.h"

#if defined PLUS_BUILD_TYPE_DEBUG
#define MARC_EXTENSION_POSTFIX "-dbg"
#elif defined PLUS_BUILD_TYPE_RELWITHDEBINFO
#define MARC_EXTENSION_POSTFIX "-rel"
#elif defined PLUS_BUILD_TYPE_RELEASE
#define MARC_EXTENSION_POSTFIX "-dist"
#else
#error PLUS_BUILD_TYPE_<CONFIG> NOT DEFINED!
#endif

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
				if (entry.path().extension().string() != PLUS_PLATFORM_PLUGIN_EXTENSION)
					continue;

				auto stem = entry.path().stem().string();
				if (stem.find(MARC_EXTENSION_POSTFIX) != stem.size() - strlen(MARC_EXTENSION_POSTFIX))
					continue;
				stem = stem.substr(0, stem.size() - strlen(MARC_EXTENSION_POSTFIX));

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
