#include "MarCmdLiveAsmInterpreter.h"

#include <iostream>
#include <fstream>
#include <cstring>

#include "PermissionGrantPrompt.h"

namespace MarCmd
{
	int LiveAsmInterpreter::run(const Settings& settings)
	{
		LiveAsmInterpreter lai(settings);
		return lai.run();
	}

	LiveAsmInterpreter::LiveAsmInterpreter(const Settings& settings)
		: m_settings(settings)
	{
		m_codeStr = "";
		m_pTokenizer = std::make_shared<MarC::AsmTokenizer>(m_codeStr);
		m_pModPack = MarC::ModulePack::create("<cin>");
		m_pModPack->tokenList = m_pTokenizer->getTokenList();
		m_pAssembler = std::make_shared<MarC::Assembler>(m_pModPack);
		m_pLinker = std::make_shared<MarC::Linker>(m_pAssembler->getModuleInfo());
		m_pInterpreter = std::make_shared<MarC::Interpreter>(m_pLinker->getExeInfo());

		for (auto& entry : m_settings.extDirs)
			m_pInterpreter->addExtDir(entry);
	}

	int LiveAsmInterpreter::run()
	{
		std::cout << "MarCmd Live MarCembly Interpreter. Enter 'exit' to exit." << std::endl;

		while (!m_pInterpreter->lastError())
		{
			std::cout << " >>> ";
			m_backupCodeStrSize = m_codeStr.size();
			m_codeStr.append(readCodeFromConsole());

			if (!m_pTokenizer->tokenize())
			{
				recover(RecoverBegin::Tokenizer);
				std::cout << "An error occured while running the tokenizer!" << std::endl
					<< "  " << m_pTokenizer->lastError().what() << std::endl;
				continue;
			}

			MarC::ModuleLoader::loadDependencies(m_pModPack->tokenList, m_pModPack->dependencies, m_settings.modDirs);

			if (!m_pAssembler->assemble())
			{
				recover(RecoverBegin::Assembler);
				std::cout << "An error occured while running the compiler!:" << std::endl
					<< "  " << m_pAssembler->lastError().what() << std::endl;
				continue;
			}

			try
			{
				if (!m_pLinker->link())
					throw m_pLinker->lastError();
			}
			catch (const MarC::LinkerError& linkErr)
			{
				recover(RecoverBegin::Linker);
				std::cout << "An error occured while running the linker!:" << std::endl
					<< "  " << linkErr.what() << std::endl;
				continue;
			}

			if (m_pInterpreter->hasUngrantedPerms())
			{
				if (m_settings.flags.hasFlag(CmdFlags::GrantAll))
				{
					m_pInterpreter->grantAllPerms();
				}
				else
				{
					std::set<std::string> toGrant;

					auto manPerms = m_pInterpreter->getUngrantedPerms(m_pInterpreter->getManPerms());
					if (!manPerms.empty())
						permissionGrantPrompt(PermissionPromptType::Mandatory, manPerms, toGrant);

					auto optPerms = m_pInterpreter->getUngrantedPerms(m_pInterpreter->getOptPerms());
					if (!optPerms.empty())
						permissionGrantPrompt(PermissionPromptType::Optional, optPerms, toGrant);

					m_pInterpreter->grantPerms(toGrant);
				}
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
						<< "    " << m_pInterpreter->lastError().what() << std::endl;
				}
			}
		}

		int64_t exitCode = m_pInterpreter->getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64;

		if (!m_settings.flags.hasFlag(CmdFlags::NoExitInfo))
		{
			std::cout << "Module '<cin>' exited with code " << exitCode << "." << std::endl;

			if (m_settings.flags.hasFlag(CmdFlags::Verbose))
				std::cout << "  Reason: '" << m_pInterpreter->lastError().what() << "'" << std::endl;
		}

		return m_pInterpreter->getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_32;
	}

	void LiveAsmInterpreter::recover(RecoverBegin rs)
	{
		switch (rs)
		{
		case RecoverBegin::Interpreter:
			std::cout << "CANNOT RECOVER FROM INTERPRETER ERRORS! IT IS RECOMMENDED RESTARTING THE PROGRAM." << std::endl;
			[[fallthrough]];
		case RecoverBegin::Linker:
			std::cout << "CANNOT RECOVER FROM LINKER ERRORS! IT IS RECOMMENDED RESTARTING THE PROGRAM." << std::endl;
			m_pAssembler->recover();
			[[fallthrough]];
		case RecoverBegin::Assembler:
			m_pTokenizer->recover();
			[[fallthrough]];
		case RecoverBegin::Tokenizer:
			m_codeStr.resize(m_backupCodeStrSize);
			[[fallthrough]];
		case RecoverBegin::None:
			break;
		}
	}

	std::string LiveAsmInterpreter::readCodeFromConsole()
	{
		std::string line;
		std::getline(std::cin, line);
		if (!line.empty())
			return line + '\n';

		if (line.find("?moddir ") == 0)
		{
			line = line.substr(strlen("%moddir "));
			m_settings.modDirs.insert(line);
			return "";
		}

		std::string code;
		do
		{
			code.append(line);
			code.push_back('\n');
			std::cout << "   > ";
			std::getline(std::cin, line);
		} while (!line.empty());

		if (code == "\n")
			return "";

		return code;
	}
}
