#include "PermissionGrantPrompt.h"

#include <iostream>

namespace MarCmd
{
	bool permissionGrantPrompt(const std::set<std::string>& toGrant, std::set<std::string>& output)
	{
		uint64_t count = 0;
		std::cout << "    (";
		for (auto& perm : toGrant)
		{
			std::cout << perm;
			if (++count < toGrant.size())
				std::cout << "|";
		}
		std::cout << ")" << std::endl;
		std::cout << "  G=grant all;c=cancel;[permlist]=perms to grant: ";

		std::string line;
		std::getline(std::cin, line);

		if (line == "g" || line.empty())
		{
			std::cout << "  Granting all permissions..." << std::endl;
			for (auto& perm : toGrant)
				output.insert(perm);
			return true;
		}

		if (line == "c")
		{
			std::cout << "  Operation canceled." << std::endl;
			return false;
		}

		std::cout << "  Granting selected permissions..." << std::endl;
		uint64_t begin = 0;
		while (begin < line.size())
		{
			uint64_t end = line.find('|', begin);
			output.insert(line.substr(begin, end - begin));
			begin = (end == std::string::npos) ? end : end + 1;
		}
		return true;
	}
}