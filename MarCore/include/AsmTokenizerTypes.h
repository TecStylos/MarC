#pragma once

#include <vector>
#include <memory>
#include <string>

namespace MarC
{
	struct AsmToken
	{
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
		} type = Type::None;
		std::string value = "";
	public:
		AsmToken() = default;
		AsmToken(Type type, const std::string& value = "")
			: type(type), value(value)
		{}
	};
	typedef std::vector<AsmToken> AsmTokenList;
	typedef std::shared_ptr<AsmTokenList> AsmTokenListRef;
}