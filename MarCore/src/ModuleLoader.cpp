#include "ModuleLoader.h"

#include "ModuleLocator.h"
#include "AsmTokenizer.h"
#include "CodeFileReader.h"

namespace MarC
{
	ModuleRef ModuleLoader::load(const std::string& modPath, const std::set<std::string>& modDirs)
	{
		ModuleRef mod = std::make_shared<Module>();

		mod->name = modNameFromPath(modPath);

		std::string source = readCodeFile(modPath);

		AsmTokenizer tokenizer(source);

		if (!tokenizer.tokenize())
			throw MarCoreException("An error occured while running the tokenizer!");

		mod->tokenList = tokenizer.getTokenList();

		loadDependencies(mod->tokenList, mod->dependencies, modDirs);

		return mod;
	}

	void ModuleLoader::loadDependencies(AsmTokenListRef tokenList, std::map<std::string, AsmTokenListRef>& dependencies, const std::set<std::string>& modDirs)
	{
		std::set<std::string> modNames;

		enum class State
		{
			Find_BeginNewline,
			Find_Directive,
			Find_DirectiveName,
			Find_Colon,
			Find_ModName,
			Find_EndNewline,
		} state = State::Find_Directive;

		for (uint64_t i = 0; i < tokenList->size(); ++i)
		{
			auto& token = (*tokenList)[i];
			switch (state)
			{
			case State::Find_BeginNewline:
				if (token.type == AsmToken::Type::Sep_Newline)
					state = State::Find_Directive;
				break;
			case State::Find_Directive:
				state = State::Find_DirectiveName;
				if (token.type != AsmToken::Type::Op_Directive)
					state = State::Find_BeginNewline;
				break;
			case State::Find_DirectiveName:
				state = State::Find_BeginNewline;
				if (token.type == AsmToken::Type::Name && token.value == "reqmod")
					state = State::Find_Colon;
				break;
			case State::Find_Colon:
				if (token.type != AsmToken::Type::Sep_Colon)
					throw MarCoreException("Expected ':' after directive 'reqmod'!");
				state = State::Find_ModName;
				break;
			case State::Find_ModName:
				if (token.type != AsmToken::Type::String)
					throw MarCoreException("Expected module name after directive 'reqmod'!");
				modNames.insert(token.value);
				state = State::Find_EndNewline;
				break;
			case State::Find_EndNewline:
				if (token.type != AsmToken::Type::Sep_Newline)
					throw MarCoreException("Expected newline after module name!");
				state = State::Find_BeginNewline;
				break;
			default:
				break;
			}
		}

		auto found = locateModules(modDirs, modNames);

		for (auto& f : found)
		{
			if (dependencies.find(f.first) != dependencies.end())
				continue;

			if (f.second.empty())
				throw MarCoreException("Unable to find module '" + f.first + "'!");
			if (f.second.size() > 1)
				throw MarCoreException("The module name '" + f.first + "' is ambigious!");
		
			std::string source = readCodeFile(*f.second.begin());

			AsmTokenizer tokenizer(source);
			
			if (!tokenizer.tokenize())
				throw MarCoreException("An error occured while running the tokenizer for a dependency!");

			dependencies.insert({ f.first, tokenizer.getTokenList() });

			loadDependencies(tokenizer.getTokenList(), dependencies, modDirs);
		}
	}
}