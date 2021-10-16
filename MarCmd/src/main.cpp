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

int main()
{
	Timer timer;

	MarC::Linker linker;
	MarC::AsmTokenizer tokenizer(readFile("testcode.mcas"));
	MarC::Compiler compiler(tokenizer.getTokenList(), linker.getExeInfo()->staticStack);
	MarC::Interpreter interpreter(linker.getExeInfo(), 4096);

	std::cout << "Adding module...";
	if (linker.addModule(compiler.getModuleInfo()))
		std::cout << " DONE" << std::endl;
	else
	{
		std::cout << std::endl << "  An error occured while adding the module to the linker!" << std::endl;
		return -1;
	}

	std::cout << "Running tokenizer...";
	if (tokenizer.tokenize())
		std::cout << " DONE" << std::endl;
	else
	{
		std::cout << std::endl << "  An error occured while running the tokenizer!" << std::endl
			<< "    " << tokenizer.lastError().getMessage() << std::endl;
		return -1;
	}

	std::cout << "Running compiler...";
	if (compiler.compile())
		std::cout << " DONE" << std::endl;
	else
	{
		std::cout << std::endl << "  An error occured while running the compiler!" << std::endl
		<< "    " << compiler.lastError().getMessage() << std::endl;
		return -1;
	}

	std::cout << "Running linker...";
	if (linker.link())
		std::cout << " DONE" << std::endl;
	else
	{
		std::cout << std::endl << "  An error occured while running the linker!" << std::endl;
		return -1;
	}

	std::cout << "Running the interpreter...";

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

	std::cout << "Execution time: " << timer.microseconds() << " microseconds" << std::endl;

	std::cout << "Exit code: " <<
		interpreter.getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64 <<
		std::endl;

	return 0;
}