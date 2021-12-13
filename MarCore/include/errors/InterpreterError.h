#pragma once

#include "MarCoreError.h"

namespace MarC
{
	class InterpreterError : public MarCoreError
	{
	public:
		enum class Code
		{
			Success,
			PlainContext,
			OpCodeUnknown,
			OpCodeNotImplemented,
			AbortViaExit,
			AbortViaEndOfCode,
			ExtensionLoadFailure,
			WrongExtCallParamCount,
			ExternalFunctionNotFound,
			PermissionDenied,
		};
	public:
		InterpreterError()
			: InterpreterError(Code::Success, "")
		{}
		InterpreterError(Code code, const std::string& context)
			: MarCoreError("InterpreterError"), m_code(code)
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
			case Code::OpCodeUnknown:
				message = "Unknown opCode '" + context + "'!";
				break;
			case Code::OpCodeNotImplemented:
				message = "The opCode '" + context + "' has not been implemented yet!";
				break;
			case Code::AbortViaExit:
				message = "The execution has been aborted through the 'exit' instruction!";
				break;
			case Code::AbortViaEndOfCode:
				message = "The execution has been aborted because the interpreter reached the end of code!";
				break;
			case Code::ExtensionLoadFailure:
				message = context;
				break;
			case Code::WrongExtCallParamCount:
				message = "Wrong number of parameters for a call to an external function!";
				break;
			case Code::ExternalFunctionNotFound:
				message = "Unable to locate external function '" + context + "'!";
				break;
			case Code::PermissionDenied:
				message = "Insufficient permissions for external function '" + context + "'!";
				break;
			default:
				message = "Unknown error code! Context: " + context;
			}

			m_whatBuff = message;
		}
	public:
		virtual explicit operator bool() const override { return m_code != Code::Success; };
	public:
		Code getCode() const { return m_code; }
		bool isOK() const
		{
			switch (m_code)
			{
			case Code::Success:
			case Code::AbortViaExit:
			case Code::AbortViaEndOfCode:
				return true;
			case Code::PlainContext:
			case Code::OpCodeUnknown:
			case Code::OpCodeNotImplemented:
			case Code::ExtensionLoadFailure:
			case Code::WrongExtCallParamCount:
			case Code::ExternalFunctionNotFound:
			case Code::PermissionDenied:
				return false;
			}
			return false;
		}
	private:
		Code m_code = Code::Success;
	};

	typedef InterpreterError::Code IntErrCode;
}