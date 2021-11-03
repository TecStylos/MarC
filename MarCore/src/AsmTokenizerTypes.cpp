#include "AsmTokenizerTypes.h"

#include <map>

namespace MarC
{
	std::string AsmTokenTypeToString(AsmToken::Type tt)
	{
		std::map<AsmToken::Type, std::string> ttMap = {
			{ AsmToken::Type::None,           "<none>" },
			{ AsmToken::Type::END_OF_CODE,    "<END_OF_CODE>" },
			{ AsmToken::Type::Sep_Dot,        "Sep_Dot" },
			{ AsmToken::Type::Sep_Colon,      "Sep_Colon" },
			{ AsmToken::Type::Sep_Newline,    "Sep_Newline" },
			{ AsmToken::Type::Op_Deref,       "Op_Deref" },
			{ AsmToken::Type::Op_FP_Relative, "Op_FP_Relative" },
			{ AsmToken::Type::Op_DT_Size,     "Op_DT_Size" },
			{ AsmToken::Type::Op_Register,    "Op_Register" },
			{ AsmToken::Type::Op_Directive,   "Op_Directive" },
			{ AsmToken::Type::Name,           "Name" },
			{ AsmToken::Type::Float,          "Float" },
			{ AsmToken::Type::Integer,        "Integer" },
			{ AsmToken::Type::String,         "String" },
			{ AsmToken::Type::Comment,        "Comment" },
		};

		auto it = ttMap.find(tt);
		if (it == ttMap.end())
			return "<unknown>";
		return it->second;
	}
}
