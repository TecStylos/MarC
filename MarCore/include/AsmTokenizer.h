#pragma once

#include <string>

#include "AsmTokenizerTypes.h"

namespace MarC
{
	class AsmTokenizerError
	{
	public:
		enum class Code
		{
			Success = 0,
			UnexpectedChar,
		};
	public:
		AsmTokenizerError() = default;
		AsmTokenizerError(Code code, uint16_t line, uint16_t column, const std::string& errText, uint64_t sysErrLine, const std::string& sysErrFile)
			: m_code(code), m_line(line), m_column(column), m_errText(errText), m_sysErrLine(sysErrLine), m_sysErrFile(sysErrFile)
		{}
	public:
		operator bool() const;
		const std::string& getText() const;
		std::string getMessage() const;
	private:
		Code m_code = Code::Success;
		uint16_t m_line = 0;
		uint16_t m_column = 0;
		std::string m_errText = "Success!";
		uint64_t m_sysErrLine = 0;
		std::string m_sysErrFile = "<unspecified>";
	};

	typedef AsmTokenizerError::Code AsmTokErrCode;

	#define ASM_TOKENIZER_THROW_ERROR(errCode, errText) throw AsmTokenizerError(errCode, currToken.line, currToken.column, errText, __LINE__, __FILE__)

	class AsmTokenizer
	{
	public:
		AsmTokenizer(const std::string& asmCode);
	public:
		bool tokenize();
	public:
		AsmTokenListRef getTokenList();
	public:
		const AsmTokenizerError& lastError() const;
	private:
		;
	private:
		std::string m_asmCode;
		AsmTokenListRef m_pTokenList;
		AsmTokenizerError m_lastErr;
		uint64_t m_nextCharToTokenize = 0;
	};
}