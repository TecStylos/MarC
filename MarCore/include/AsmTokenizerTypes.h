#pragma once

#include <vector>
#include <memory>
#include <string>

namespace MarC
{
	struct AsmToken
	{
		uint16_t line;
		uint16_t column;
		enum class Type
		{
			None = 0,
			END_OF_CODE,
			Sep_Dot,
			Sep_Colon,
			Sep_Newline,
			Op_Deref,
			Op_FP_Relative,
			Op_Register,
			Op_Directive,
			Name,
			Float,
			Integer,
			String,
			Comment,
		} type;
		std::string value;
	public:
		AsmToken(uint16_t line, uint16_t column, Type type = Type::None, const std::string& value = "")
			: line(line), column(column), type(type), value(value)
		{}
	};
	typedef std::vector<AsmToken> AsmTokenList;
	typedef std::shared_ptr<AsmTokenList> AsmTokenListRef;

	std::string AsmTokenTypeToString(AsmToken::Type tt);
}