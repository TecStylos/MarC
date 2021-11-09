#include "MarCmdLiveAsmInterpreter.h"

#include <iostream>
#include <fstream>
#include <cstring>

namespace MarCmd
{
	int LiveAsmInterpreter::run(const std::set<std::string>& modDirs, const std::set<std::string>& extDirs, Flags<CmdFlags> flags)
	{
		LiveAsmInterpreter lai(modDirs, extDirs, flags);
		return lai.run();
	}

	LiveAsmInterpreter::LiveAsmInterpreter(const std::set<std::string>& modDirs, const std::set<std::string>& extDirs, Flags<CmdFlags> flags)
		: m_modDirs(modDirs), m_flags(flags)
	{
		m_codeStr = "";
		m_pTokenizer = std::make_shared<MarC::AsmTokenizer>(m_codeStr);
		m_pCompiler = std::make_shared<MarC::Compiler>(m_pTokenizer->getTokenList(), "<cin>");
		m_pLinker = std::make_shared<MarC::Linker>();
		m_pInterpreter = std::make_shared<MarC::Interpreter>(m_pLinker->getExeInfo());

		for (auto& entry : extDirs)
			m_pInterpreter->addExtDir(entry);
	
		m_pLinker->addModule(m_pCompiler->getModuleInfo());
	}

	int LiveAsmInterpreter::run()
	{
		std::cout << "MarCmd Live MarCembly Interpreter" << std::endl;

		while (!m_pInterpreter->lastError())
		{
			std::cout << " >>> ";
			m_backupCodeStrSize = m_codeStr.size();
			m_codeStr.append(readCodeFromConsole());

			if (!m_pTokenizer->tokenize())
			{
				recover(RecoverBegin::Tokenizer);
				std::cout << "An error occured while running the tokenizer!" << std::endl
					<< "  " << m_pTokenizer->lastError().getMessage() << std::endl;
				continue;
			}

			if (!m_pCompiler->compile())
			{
				recover(RecoverBegin::Compiler);
				std::cout << "An error occured while running the compiler!:" << std::endl
					<< "  " << m_pCompiler->lastError().getMessage() << std::endl;
				continue;
			}

			if (!m_pLinker->update())
			{
				std::cout << "An error occured while updating the linker information:" << std::endl
					<< "  " << m_pLinker->lastError().getMessage() << std::endl;
				continue;
			}

			bool foundAllMissingModules = true;

			while (foundAllMissingModules && m_pLinker->hasMissingModules())
			{
				auto& misMods = m_pLinker->getMissingModules();
				auto modPaths = MarC::locateModules(m_modDirs, misMods);

				for (auto& pair : modPaths)
				{
					if (pair.second.empty())
					{
						std::cout << "Unable to find module '" << pair.first << "'!" << std::endl;
						foundAllMissingModules = false;
						break;
					}
					if (pair.second.size() > 1)
					{
						std::cout << "Module name '" << pair.first << "' is ambigious! Found " << pair.second.size() << " matching modules!" << std::endl;
						for (auto& p : pair.second)
							std::cout << "  " << p << std::endl;
						foundAllMissingModules = false;
						break;
					}

					if (!addModule(*m_pLinker, *pair.second.begin(), pair.first, m_flags.hasFlag(CmdFlags::Verbose)))
					{
						foundAllMissingModules = false;
						break;
					}
				}
			}

			if (!foundAllMissingModules)
			{
				recover(RecoverBegin::Linker);
				continue;
			}

			if (!m_pLinker->link())
			{
				recover(RecoverBegin::Linker);
				std::cout << "An error occured while running the linker!" << std::endl
					<< "  " << m_pLinker->lastError().getMessage() << std::endl;
				continue;
			}

			if (!m_pInterpreter->interpret())
			{
				if (m_pInterpreter->lastError().getCode() == MarC::IntErrCode::AbortViaEndOfCode)
				{
					m_pInterpreter->resetError();
				}
				if (!m_pInterpreter->lastError().isOK())
				{
					recover(RecoverBegin::Interpreter);
					std::cout << std::endl << "An error occured while interpreting the code!" << std::endl
						<< "    " << m_pInterpreter->lastError().getMessage() << std::endl;
				}
			}
		}

		int64_t exitCode = m_pInterpreter->getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64;

		std::cout << "Module '<cin>' exited with code " << exitCode << "." << std::endl;

		if (m_flags.hasFlag(CmdFlags::Verbose))
			std::cout << "  Reason: '" << m_pInterpreter->lastError().getCodeStr() << "'" << std::endl;

		return m_pInterpreter->getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_32;
	}

	void LiveAsmInterpreter::recover(RecoverBegin rs)
	{
		switch (rs)
		{
		case RecoverBegin::Interpreter:
			std::cout << "CANNOT RECOVER FROM INTERPRETER ERRORS! IT IS RECOMMENDED RESTARTING THE PROGRAM." << std::endl;
			//__fallthrough
		case RecoverBegin::Linker:
			std::cout << "CANNOT RECOVER FROM LINKER ERRORS! IT IS RECOMMENDED RESTARTING THE PROGRAM." << std::endl;
			m_pCompiler->recover();
			//__fallthrough
		case RecoverBegin::Compiler:
			m_pTokenizer->recover();
			//__fallthrough
		case RecoverBegin::Tokenizer:
			m_codeStr.resize(m_backupCodeStrSize);
			//__fallthrough
		case RecoverBegin::None:
			break;
		}
	}

	std::string LiveAsmInterpreter::readCodeFromConsole()
	{
		std::string line;
		std::getline(std::cin, line);
		if (line.empty() || line[0] != '%')
			return line + '\n';

		if (line.size() > 1)
		{
			if (line.find("%moddir ") == 0)
			{
				line = line.substr(strlen("%moddir "));
				m_modDirs.insert(line);
				return "";
			}
			std::cout << "Unknown live interpreter command!" << std::endl;
		}

		std::cout << "   > ";
		std::getline(std::cin, line);
		std::string code;
		do
		{
			code.append(line);
			code.push_back('\n');
			std::cout << "   > ";
			std::getline(std::cin, line);
		} while (line != "%");

		return code;
	}

	std::string LiveAsmInterpreter::readFile(const std::string& filepath)
	{
		std::ifstream f(filepath);
		if (!f.good())
			return "";

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

		return result;
	}

	bool LiveAsmInterpreter::addModule(MarC::Linker& linker, const std::string& modPath, const std::string& modName, bool verbose)
	{
		std::string codeStr = readFile(modPath);
		MarC::AsmTokenizer tokenizer(codeStr);
		MarC::Compiler compiler(tokenizer.getTokenList(), modName);

		if (verbose)
			std::cout << "Tokenizing module '" << modName << "'..." << std::endl;
		if (!tokenizer.tokenize())
		{
			std::cout << "An error occured while running the tokenizer!" << std::endl
				<< "  " << tokenizer.lastError().getMessage() << std::endl;
			return false;
		}

		if (verbose)
			std::cout << "Compiling module '" << modName << "'..." << std::endl;
		if (!compiler.compile())
		{
			std::cout << "An error occured while running the compiler!:" << std::endl
				<< "  " << compiler.lastError().getMessage() << std::endl;
			return false;
		}

		if (verbose)
			std::cout << "Adding module '" << compiler.getModuleInfo()->moduleName << "' to the linker..." << std::endl;
		if (!linker.addModule(compiler.getModuleInfo()))
		{
			std::cout << "An error occurd while adding the module '" << compiler.getModuleInfo()->moduleName << "' to the linker!" << std::endl;
			return false;
		}

		return true;
	}
}
