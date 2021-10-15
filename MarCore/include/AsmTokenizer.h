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
		AsmTokenizerError(Code code, uint64_t errLine, const std::string& errText, uint64_t sysErrLine, const std::string& sysErrFile)
			: m_code(code), m_errLine(errLine), m_errText(errText), m_sysErrLine(sysErrLine), m_sysErrFile(sysErrFile)
		{}
	public:
		operator bool() const;
		const std::string& getText() const;
		std::string getMessage() const;
	private:
		Code m_code = Code::Success;
		uint64_t m_errLine = 0;
		std::string m_errText = "Success!";
		uint64_t m_sysErrLine = 0;
		std::string m_sysErrFile = "<unspecified>";
	};

	typedef AsmTokenizerError::Code AsmTokErrCode;

	#define ASM_TOKENIZER_RETURN_WITH_ERROR(errCode, errText) { m_lastErr = AsmTokenizerError(errCode, -1, errText, __LINE__, __FILE__); return false; }
	#define ASM_TOKENIZER_THROW_ERROR(errCode, errText) throw AsmTokenizerError(errCode, -1, errText, __LINE__, __FILE__)

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