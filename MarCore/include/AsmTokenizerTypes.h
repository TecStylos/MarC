#pragma once

#include <vector>
#include <memory>
#include <string>

namespace MarC
{
	struct AsmToken
	{
		uint16_t line = 0;
		uint16_t column = 0;
		enum class Type
		{
			None = 0,
			END_OF_CODE,
			Sep_Dot,
			Sep_Colon,
			Sep_Newline,
			Op_Deref,
			Op_FP_Relative,
			Op_DT_Size,
			Op_Register,
			Op_Directive,
			Op_Ignore_Directive,
			Spec_NoLocal,
			Name,
			Float,
			Integer,
			String,
			Pragma_Insertion,
			Comment,
		} type = Type::None;
		std::string value = "<undefined>";
	public:
		AsmToken() = default;
		AsmToken(uint16_t line, uint16_t column, Type type = Type::None, const std::string& value = "")
			: line(line), column(column), type(type), value(value)
		{}
	public:
		bool operator<(const AsmToken& other) const { return value < other.value; }
		bool operator>(const AsmToken& other) const { return value > other.value; }
	};
	typedef std::vector<AsmToken> AsmTokenList;
	typedef std::shared_ptr<AsmTokenList> AsmTokenListRef;

	std::string AsmTokenTypeToString(AsmToken::Type tt);
}