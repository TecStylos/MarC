#pragma once

#include <set>
#include <string>

#include "MarCore.h"

#include "MarCmdFlags.h"

namespace MarCmd
{
	class LiveAsmInterpreter
	{
	public:
		static int run(const std::set<std::string>& modDirs, const std::set<std::string>& extDirs, Flags<CmdFlags> flags);
	private:
		LiveAsmInterpreter(const std::set<std::string>& modDirs, const std::set<std::string>& extDirs, Flags<CmdFlags> flags);
	private:
		int run();
	private:
		enum class RecoverBegin
		{
			None,
			Tokenizer,
			Compiler,
			Linker,
			Interpreter,
		};
		void recover(RecoverBegin rs);
		std::string readCodeFromConsole();
		static std::string readFile(const std::string& filepath);
		static bool addModule(MarC::Linker& linker, const std::string& modPath, const std::string& modName, bool verbose);
	private:
		std::set<std::string> m_modDirs;
		Flags<CmdFlags> m_flags;
	private:
		uint64_t m_backupCodeStrSize = 0;
		std::string m_codeStr;
		std::shared_ptr<MarC::AsmTokenizer> m_pTokenizer;
		std::shared_ptr<MarC::Compiler> m_pCompiler;
		std::shared_ptr<MarC::Linker> m_pLinker;
		std::shared_ptr<MarC::Interpreter> m_pInterpreter;
	};
}