#pragma once

#include "AsmTokenizerTypes.h"

namespace MarC
{
	struct Macro
	{
		std::vector<AsmToken> parameters;
		AsmTokenList tokenList;
	};
}