#include "MarCmdBuilder.h"

#include <fstream>
#include <filesystem>
#include <MarCore.h>

#include "AutoExecutableLoader.h"

namespace MarCmd
{
	int Builder::run(const Settings& settings)
	{
		bool verbose = settings.flags.hasFlag(CmdFlags::Verbose);

		auto exeInfo = autoLoadExecutable(settings.inFile, settings.modDirs);

		std::string outFile;
		if (!settings.outFile.empty())
			outFile = settings.outFile;
		else
			outFile = std::filesystem::path(settings.inFile).replace_extension(".mce").string();

		std::ofstream oStream(outFile, std::ios::binary | std::ios::out | std::ios::trunc);
		if (!oStream.is_open())
		{
			std::cout << "Unable to open the output file!" << std::endl;
			return -1;
		}

		if (verbose)
			std::cout << "Writing executable to disk..." << std::endl;
		MarC::serialize(*exeInfo, oStream);

		if (!oStream.good())
		{
			std::cout << "An error occured while writing the executable to disk!" << std::endl;
			return -1;
		}

		oStream.close();

		std::cout << "Successfully built the application!" << std::endl;

		return 0;
	}
}