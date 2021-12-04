#include "MarCmdBuilder.h"

#include <fstream>
#include <filesystem>
#include <MarCore.h>

#include "MarCmdModuleAdder.h"

namespace MarCmd
{
	int Builder::run(const Settings& settings)
	{
		MarC::Linker linker(settings.modDirs);

		bool verbose = settings.flags.hasFlag(CmdFlags::Verbose);

		try
		{
			if (verbose)
				std::cout << "Adding input file to the linker..." << std::endl;
			addModule(linker, settings.inFile, modNameFromPath(settings.inFile), &verbose);

			if (verbose)
				std::cout << "Adding missing modules to the linker..." << std::endl;
			linker.autoAddMissingModules(&addModule, &verbose);
		}
		catch (const MarC::AsmTokenizerError& err)
		{
			std::cout << "An error occured while running the tokenizer!:" << std::endl
				<< "  " << err.getMessage() << std::endl;
			return -1;
		}
		catch (const MarC::AssemblerError& err)
		{
			std::cout << "An error occured while runing the assembler!" << std::endl
				<< "  " << err.getMessage() << std::endl;
			return -1;
		}
		catch (const MarC::LinkerError& err)
		{
			std::cout << "An error occured while running the linker!:" << std::endl
				<< "  " << err.getMessage() << std::endl;
			return -1;
		}
		catch (const std::runtime_error& err)
		{
			std::cout << "An unexpected error occured!:" << std::endl
				<< "  " << err.what() << std::endl;
			return -1;
		}

		if (!linker.link())
		{
			std::cout << "An error occured while running the linker!:" << std::endl
				<< "  " << linker.lastError().getMessage() << std::endl;
			return -1;
		}

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

		auto exeInfo = linker.getExeInfo();
		exeInfo->hasDebugInfo = settings.flags.hasFlag(CmdFlags::DebugInfo);

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