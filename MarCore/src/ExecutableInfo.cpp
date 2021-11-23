#include "ExecutableInfo.h"

namespace MarC
{
	struct ExeInfoHeader
	{
		uint64_t nSymbols;
		uint64_t nManPerms;
		uint64_t nOptPerms;
		uint64_t nModules;
	};

	std::ostream& operator<<(std::ostream& outStream, const ExecutableInfo& eInfo)
	{
		ExeInfoHeader header;
		header.nSymbols = eInfo.symbols.size();
		header.nManPerms = eInfo.mandatoryPermissions.size();
		header.nOptPerms = eInfo.optionalPermissions.size();
		header.nModules = eInfo.modules.size();

		outStream.write((const char*)&header, sizeof(header));

		for (auto& symbol : eInfo.symbols)
		{
			outStream.write(symbol.name.c_str(), symbol.name.size() + 1);
			outStream.write((const char*)&symbol.usage, sizeof(symbol.usage));
			outStream.write((const char*)&symbol.value, sizeof(symbol.value));
		}

		for (auto& perm : eInfo.mandatoryPermissions)
			outStream.write(perm.c_str(), perm.size() + 1);

		for (auto& perm : eInfo.optionalPermissions)
			outStream.write(perm.c_str(), perm.size() + 1);

		for (auto& mod : eInfo.modules)
			outStream << *mod;

		return outStream;
	}

	std::istream& operator>>(std::istream& inStream, ExecutableInfo& eInfo)
	{
		eInfo = ExecutableInfo();
		return inStream;
	}
}