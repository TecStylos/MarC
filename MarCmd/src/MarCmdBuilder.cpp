#include "MarCmdBuilder.h"

#include <fstream>
#include <filesystem>
#include <MarCore.h>

#include "MarCmdModuleAdder.h"

namespace MarCmd
{
	int Builder::run(const Settings& settings)
	{
		bool verbose = settings.flags.hasFlag(CmdFlags::Verbose);

		auto mod = MarC::ModuleLoader::load(settings.inFile, settings.modDirs);

		MarC::Assembler assembler(mod);
		if (!assembler.assemble())
			throw std::runtime_error("Unable to assemble the modules!");

		MarC::Linker linker(assembler.getModuleInfo());
		if (!linker.link())
			throw std::runtime_error("Unable to link the modules!");

		auto exeInfo = linker.getExeInfo();

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