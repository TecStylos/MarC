#include <MarCore.h>

#include <iostream>
#include <fstream>

#include <Windows.h>

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

	return result;
}

int main()
{
	std::string code = readFile("testcode.mcas"); // Load the MarC Assembly file

	MarC::BytecodeInfo bci;

	MarC::AssemblerInfo asmInfo;
	asmInfo.nextCharToAssemble = 0;
	asmInfo.pAssemblyCode = &code;

	MarC::AssemblerError asmErr;

	if (!MarC::Assembler::assemble(bci, asmInfo, asmErr))
		std::cout << "An error occured while running the assembler!" << std::endl
		<< "    " << asmErr.getMessage() << std::endl;
	else
		std::cout << "Successfully assembled the code!" << std::endl;

	return 0;
}