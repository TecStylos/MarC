#include <MarCore.h>

#include <iostream>
#include <fstream>
#include <chrono>

class Timer
{
public:
	void start() { m_start = std::chrono::high_resolution_clock::now(); }
	void stop() { m_stop = std::chrono::high_resolution_clock::now(); }
	uint64_t microseconds() { return std::chrono::duration_cast<std::chrono::microseconds>(m_stop - m_start).count(); }
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_stop;
};

std::string readFile(const std::string& filepath)
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

bool addModule(MarC::Linker& linker, const std::string& modPath, const std::string& modName)
{
	MarC::AsmTokenizer tokenizer(readFile(modPath));
	MarC::Compiler compiler(tokenizer.getTokenList(), modName);

	std::cout << "Tokenizing '" << modPath << "'...";
	if (tokenizer.tokenize())
		std::cout << " DONE" << std::endl;
	else
	{
		std::cout << "  ERROR" << std::endl
			<< "    " << tokenizer.lastError().getMessage() << std::endl;
		return false;
	}

	std::cout << "Compiling '" << modPath << "'...";
	if (compiler.compile())
		std::cout << " DONE" << std::endl;
	else
	{
		std::cout << "  ERROR" << std::endl
			<< "    " << compiler.lastError().getMessage() << std::endl;
		return false;
	}

	std::cout << "Adding module '" << modName << "' to linker...";
	if (linker.addModule(compiler.getModuleInfo()))
		std::cout << " DONE" << std::endl;
	else
	{
		std::cout << "  ERROR" << std::endl;
		return false;
	}

	return true;
}

int main()
{
	Timer timer;

	std::string inclDir = "../examples/";

	MarC::Linker linker;
	MarC::Interpreter interpreter(linker.getExeInfo(), 4096);

	addModule(linker, "../examples/copyString.mca", "copyString");

	while (linker.hasMissingModules())
	{
		auto& modName = linker.getMissingModule();
		std::string modPath = MarC::locateModule(inclDir, modName);
		if (!modPath.empty())
		{
			addModule(linker, modPath, modName);
		}
	}

	std::cout << "Linking the application...";
	if (linker.link())
		std::cout << " DONE" << std::endl;
	else
	{
		std::cout << "  ERROR" << std::endl;
		return -1;
	}

	std::cout << "Running interpreter...";

	timer.start();
	bool intResult = interpreter.interpret();
	timer.stop();
	if (intResult || interpreter.lastError().isOK())
		std::cout << " DONE" << std::endl;
	else
	{
		std::cout << std::endl << "  An error occured while interpreting the code!" << std::endl
		<< "    " << interpreter.lastError().getMessage() << std::endl;
		return -1;
	}

	std::cout << "Executed " << interpreter.nInsExecuted() << " instructions in " << timer.microseconds() << " microseconds" << std::endl;

	std::cout << "Exit code: " <<
		interpreter.getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64 <<
		std::endl;

	return 0;
}