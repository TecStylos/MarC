#pragma once

#include "MarCoreError.h"

namespace MarC
{
	class AsmTokenizerError : public MarCoreError
	{
	public:
		enum class Code
		{
			Success,
			PlainContext,
			UnexpectedChar,
		};
	public:
		AsmTokenizerError()
			: AsmTokenizerError(Code::Success, 0, 0, "")
		{}
		AsmTokenizerError(Code code, uint16_t line, uint16_t column, const std::string& context)
			: MarCoreError("AsmTokenizerError", context), m_code(code)
		{
			std::string message;

			switch (m_code)
			{
			case Code::Success:
				message = "Success";
				break;
			case Code::PlainContext:
				message = context;
				break;
			case Code::UnexpectedChar:
				message = context;
				break;
			default:
				message = "Unknown error code! Context: " + context;
			}

			m_whatBuff = "(" + std::to_string(line) + "; " + std::to_string(column) + "): " + message;
		}
	public:
		virtual explicit operator bool() const override { return m_code != Code::Success; }
	private:
		Code m_code = Code::Success;
	};

	typedef AsmTokenizerError::Code AsmTokErrCode;

	#define ASM_TOKENIZER_THROW_ERROR(errCode, context) throw AsmTokenizerError(errCode, currToken.line, currToken.column, context)
}