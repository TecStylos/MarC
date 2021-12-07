#include "CodeFileReader.h"

#include <string>
#include <fstream>
#include <filesystem>

namespace MarC
{
	std::string modNameFromPath(const std::string& filepath)
	{
		return std::filesystem::path(filepath).stem().string();
	}

	std::string modExtFromPath(const std::string& filepath)
	{
		return std::filesystem::path(filepath).extension().string();
	}

	std::string readCodeFile(const std::string& filepath)
	{
		std::ifstream f(filepath);
		if (!f.good())
			throw MarCoreException("Unable to open the code file!");

		std::string result;
		while (!f.eof())
		{
			std::string line;
			std::getline(f, line);
			result.append(line);
			result.push_back('\n');
		}
		if (!result.empty())
			result.pop_back();

		if (f.fail())
			throw MarCoreException("An error occured while reading the input file!");

		return result;
	}
}