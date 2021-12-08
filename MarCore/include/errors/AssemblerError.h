#pragma once

#include "MarCoreError.h"

namespace MarC
{
	class AssemblerError : public MarCoreError
	{
	public:
		enum class Code
		{
			Success,
			PlainContext,
			UnexpectedToken,
			UnknownInsArgType,
			DatatypeMismatch,
			UnknownRegisterName,
			UnknownDirective,
			AlreadyInGlobalScope,
			InternalError,
			InvalidScope,
			NotSpecialized,
		};
	public:
		AssemblerError()
			: MarCoreError("AssemblerError")
		{
			m_whatBuff = "SUCCESS";
		}
		AssemblerError(Code code, uint64_t line, uint64_t column, const std::string& context)
			: MarCoreError("AssemblerError", context), m_code(code), m_line(line), m_column(column)
		{
			switch (m_code)
			{
			case Code::Success:
				m_whatBuff = "Success";
				break;
			case Code::PlainContext:
				m_whatBuff = context;
				break;
			case Code::UnexpectedToken:
			{
				uint64_t sep = context.find('|');
				std::string expectedType = sep != std::string::npos ? context.substr(0, sep) : "";
				std::string gotType = sep != std::string::npos ? context.substr(sep + 1) : "";
				m_whatBuff = "Expected token of type '" + expectedType + "', but got '" + gotType + "'!";
				break;
			}
			case Code::UnknownInsArgType:
				m_whatBuff = "Cannot handle instruction argument type '" + context + "'!";
				break;
			case Code::DatatypeMismatch:
				m_whatBuff = "Expected value of datatype '" + context + "'!";
				break;
			case Code::UnknownRegisterName:
				m_whatBuff = "Unknown register name '" + context + "'!";
				break;
			case Code::UnknownDirective:
				m_whatBuff = "Unknown directive '" + context + "'!";
				break;
			case Code::AlreadyInGlobalScope:
				m_whatBuff = "Cannot end scope, already in global scope!";
				break;
			case Code::InternalError:
				m_whatBuff = "Internal Error: " + context;
				break;
			case Code::InvalidScope:
				m_whatBuff = "Invalid Scope: " + context;
				break;
			case Code::NotSpecialized:
				m_whatBuff = "The opCode '" + context + "' is not specialized!";
			default:
				m_whatBuff = "Unknown error code! Context: " + context;
			}
		}
	public:
		virtual const char* what() const noexcept
		{
			return m_whatBuff.c_str();
		}
	public:
		virtual explicit operator bool() const { return m_code != Code::Success; }
	private:
		Code m_code = Code::Success;
		uint64_t m_line = 0;
		uint64_t m_column = 0;
		std::string m_whatBuff;
	};

	typedef AssemblerError::Code AsmErrCode;

	#define MARC_ASSEMBLER_THROW(code, context) throw AssemblerError(code, currToken().line, currToken().column, context)
	#define MARC_ASSEMBLER_THROW_NO_CONTEXT(code) MARC_ASSEMBLER_THROW(code, "")

	#define MARC_ASSEMBLER_THROW_UNEXPECTED_TOKEN(expectedType, token) \
	MARC_ASSEMBLER_THROW(AsmErrCode::UnexpectedToken, AsmTokenTypeToString(expectedType) + "|" + AsmTokenTypeToString(token.type))
}