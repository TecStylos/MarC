#pragma once

#include "AsmTokenizerTypes.h"

namespace MarC
{
	struct Macro
	{
		std::vector<std::string> parameters;
		AsmTokenListRef tokenList;
	};
}