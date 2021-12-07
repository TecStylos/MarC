#include "AutoExecutableLoader.h"

namespace MarCmd
{
	MarC::ExecutableInfoRef autoLoadExecutable(const std::string& inFile, const std::set<std::string>& modDirs)
	{
		MarC::ExecutableInfoRef exeInfo = nullptr;
		auto extension = MarC::modExtFromPath(inFile);
		if (extension == ".mcc")
		{
			throw std::runtime_error("MarC code files are not supported yet!");
		}
		else if (extension == ".mce")
		{
			exeInfo = MarC::ExecutableLoader::load(inFile);
		}
		else if (extension == ".mca")
		{
			auto mod = MarC::ModuleLoader::load(inFile, modDirs);

			MarC::Assembler assembler(mod);
			if (!assembler.assemble())
				throw std::runtime_error("Unable to assemble the modules!");

			MarC::Linker linker(assembler.getModuleInfo());
			if (!linker.link())
				throw std::runtime_error("Unable to link the modules!");

			exeInfo = linker.getExeInfo();
		}
		else
		{
			throw std::runtime_error("Unknown input file format!");
		}
		return exeInfo;
	}
}