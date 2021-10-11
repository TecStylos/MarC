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
	MarC::Assembler assembler(readFile("testcode.mcas"), linker.getExeInfo()->staticStack);
	MarC::Interpreter interpreter(linker.getExeInfo(), 4096);

	if (assembler.assemble())
		std::cout << "Successfully assembled the code!" << std::endl;
	else
	{
		std::cout << "An error occured while running the assembler!" << std::endl
		<< "    " << assembler.lastError().getMessage() << std::endl;
		return -1;
	}

	linker.addModule(assembler.getModuleInfo());

	if (linker.link())
		std::cout << "Successfully linked the code!" << std::endl;
	else
	{
		std::cout << "An error occured while running the linker!" << std::endl;
		return -1;
	}

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