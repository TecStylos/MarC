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
			: AssemblerError(Code::Success, 0, 0, "")
		{}
		AssemblerError(Code code, uint64_t line, uint64_t column, const std::string& context)
			: MarCoreError("AssemblerError", context), m_code(code)
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
			case Code::UnexpectedToken:
			{
				uint64_t sep = context.find('|');
				std::string expectedType = sep != std::string::npos ? context.substr(0, sep) : "";
				std::string gotType = sep != std::string::npos ? context.substr(sep + 1) : "";
				message = "Expected token of type '" + expectedType + "', but got '" + gotType + "'!";
				break;
			}
			case Code::UnknownInsArgType:
				message = "Cannot handle instruction argument type '" + context + "'!";
				break;
			case Code::DatatypeMismatch:
				message = "Expected value of datatype '" + context + "'!";
				break;
			case Code::UnknownRegisterName:
				message = "Unknown register name '" + context + "'!";
				break;
			case Code::UnknownDirective:
				message = "Unknown directive '" + context + "'!";
				break;
			case Code::AlreadyInGlobalScope:
				message = "Cannot end scope, already in global scope!";
				break;
			case Code::InternalError:
				message = "Internal Error: " + context;
				break;
			case Code::InvalidScope:
				message = "Invalid Scope: " + context;
				break;
			case Code::NotSpecialized:
				message = "The opCode '" + context + "' is not specialized!";
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

	typedef AssemblerError::Code AsmErrCode;

	#define MARC_ASSEMBLER_THROW(code, context) throw AssemblerError(code, currToken().line, currToken().column, context)
	#define MARC_ASSEMBLER_THROW_NO_CONTEXT(code) MARC_ASSEMBLER_THROW(code, "")

	#define MARC_ASSEMBLER_THROW_UNEXPECTED_TOKEN(expectedType, token) \
	MARC_ASSEMBLER_THROW(AsmErrCode::UnexpectedToken, AsmTokenTypeToString(expectedType) + "|" + AsmTokenTypeToString(token.type))
}