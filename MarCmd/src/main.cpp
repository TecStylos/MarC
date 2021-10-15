#include <MarCore.h>

#include <iostream>
#include <fstream>

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
	MarC::Linker linker;
	MarC::AsmTokenizer tokenizer(readFile("testcode.mcas"));
	MarC::Compiler compiler(tokenizer.getTokenList(), linker.getExeInfo()->staticStack);
	MarC::Interpreter interpreter(linker.getExeInfo(), 4096);

	if (tokenizer.tokenize())
		std::cout << "Successfully tokenized the code!" << std::endl;
	else
	{
		std::cout << "An error occured while running the tokenizer!" << std::endl
			<< "    " << tokenizer.lastError().getMessage() << std::endl;
		return -1;
	}

	if (compiler.compile())
		std::cout << "Successfully compiled the code!" << std::endl;
	else
	{
		std::cout << "An error occured while running the compiler!" << std::endl
		<< "    " << compiler.lastError().getMessage() << std::endl;
		return -1;
	}

	if (linker.addModule(compiler.getModuleInfo()))
		std::cout << "Successfully added the module to the linker!" << std::endl;
	else
	{
		std::cout << "An error occured while adding the module to the linker!" << std::endl;
		return -1;
	}

	if (linker.link())
		std::cout << "Successfully linked the code!" << std::endl;
	else
	{
		std::cout << "An error occured while running the linker!" << std::endl;
		return -1;
	}

	std::cout << "Running the interpreter..." << std::endl;
	if (interpreter.interpret() || interpreter.lastError().isOK())
		std::cout << "Successfully interpreted the code!" << std::endl;
	else
	{
		std::cout << "An error occured while interpreting the code!" << std::endl
		<< "    " << interpreter.lastError().getMessage() << std::endl;
		return -1;
	}

	std::cout << "Exit code: " <<
		interpreter.getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64 <<
		std::endl;

	return 0;
}