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
	std::string code = readFile("testcode.mcas"); // Load the MarC Assembly file

	MarC::ModuleInfo modInfo;
	modInfo.staticStack = std::make_shared<MarC::Memory>();
	modInfo.codeMemory = std::make_shared<MarC::Memory>();

	MarC::AssemblerInfo asmInfo;
	asmInfo.nextCharToAssemble = 0;
	asmInfo.pAssemblyCode = &code;

	MarC::AssemblerError asmErr;

	if (!MarC::Assembler::assemble(modInfo, asmInfo, asmErr))
		std::cout << "An error occured while running the assembler!" << std::endl
		<< "    " << asmErr.getMessage() << std::endl;
	else
		std::cout << "Successfully assembled the code!" << std::endl;

	MarC::Interpreter ip(modInfo.staticStack, modInfo.codeMemory, 4096);

	if (!ip.interpret() && ip.lastError().getCode() != MarC::IntErrCode::AbortViaExit)
		std::cout << "An error occured while interpreting the code!" << std::endl
		<< "    " << ip.lastError().getMessage() << std::endl;
	else
		std::cout << "Successfully interpreted the code!" << std::endl;

	std::cout << "Exit code: " << ip.getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_U_64 << std::endl;

	MarC::BC_MemAddress addr;
	addr = *(MarC::BC_MemAddress*)ip.hostAddress(MarC::BC_MemAddress(MarC::BC_MEM_BASE_DYNAMIC_STACK, 0));
	std::cout << "Extern pointer base: " << addr.base << std::endl;

	return 0;
}