#include "MarCmdExeInfoLoader.h"

namespace MarCmd
{
	MarC::ExecutableInfoRef loadExeInfo(const Settings& settings)
	{
		bool verbose = settings.flags.hasFlag(CmdFlags::Verbose);

		MarC::ExecutableInfoRef exeInfo;
		std::string inMod = modNameFromPath(settings.inFile);
		auto extension = std::filesystem::path(settings.inFile).extension().string();

		if (extension == ".mcc")
			throw std::runtime_error("Interpreting *.mcc files is not supported at the moment!");
		else if (extension == ".mce")
		{
			std::ifstream iStream(settings.inFile, std::ios::binary | std::ios::in);
			if (!iStream.is_open())
			{
				std::cout << "Unable to open input file!" << std::endl;
				return nullptr;
			}
			exeInfo = MarC::ExecutableInfo::create();

			if (verbose)
				std::cout << "Loading input file from disk..." << std::endl;
			MarC::deserialize(*exeInfo, iStream);

			if (!iStream.good())
			{
				std::cout << "An error occured while reading the application from disk!" << std::endl;
				return nullptr;
			}
		}
		else if (extension == ".mca")
		{
			//MarC::Linker linker(settings.modDirs);
			MarC::Linker linker(nullptr);

			try
			{
				addModule(linker, settings.inFile, inMod, &verbose);
			}
			catch (const MarC::AsmTokenizerError& err)
			{
				std::cout << "An error occured while running the tokenizer!:" << std::endl
					<< "  " << err.getMessage() << std::endl;
				return nullptr;
			}
			catch (const MarC::AssemblerError& err)
			{
				std::cout << "An error occured while running the assembler!" << std::endl
					<< "  " << err.getMessage() << std::endl;
				return nullptr;
			}
			catch (const MarC::LinkerError& err)
			{
				std::cout << "An error occured while running the linker!:" << std::endl
					<< "  " << err.getMessage() << std::endl;
				return nullptr;
			}
			catch (const std::runtime_error& err)
			{
				std::cout << "An unexpected error occured!:" << std::endl
					<< "  " << err.what() << std::endl;
				return nullptr;
			}

			//if (!linker.autoAddMissingModules(&addModule, &verbose))
			{
				std::cout << "An error occured while auto adding the required modules!:" << std::endl
					<< "  " << linker.lastError().getMessage() << std::endl;
				return nullptr;
			}

			if (settings.flags.hasFlag(CmdFlags::Verbose))
				std::cout << "Linking the application..." << std::endl;
			if (!linker.link())
			{
				std::cout << "An error occured while running the linker!" << std::endl
					<< "  " << linker.lastError().getMessage() << std::endl;
				return nullptr;
			}

			exeInfo = linker.getExeInfo();
		}
		else
		{
			std::cout << "Unsupported input file!" << std::endl;
			return nullptr;
		}

		return exeInfo;
	}
}