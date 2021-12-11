#pragma once

#include "MarCoreError.h"

namespace MarC
{
	class LinkerError : public MarCoreError
	{
	public:
		enum class Code
		{
			Success = 0,
			PlainContext,
			ModuleAlreadyExisting,
			MissingModules,
			SymbolAlreadyDefined,
			UnresolvedSymbols,
			ModuleNotFound,
			AmbigiousModule,
		};
	public:
		LinkerError()
			: LinkerError(Code::Success, "")
		{}
		LinkerError(Code code, const std::string& context)
			: MarCoreError("LinkerError"), m_code(code)
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
			case Code::ModuleAlreadyExisting:
				message = "The module '" + context + "' already exists!";
				break;
			case Code::MissingModules:
				message = "Some modules are missing!";
				break;
			case Code::SymbolAlreadyDefined:
				message = "A symbol with the name '" + context + "' has already been defined!";
				break;
			case Code::UnresolvedSymbols:
				message = "Unable to resolve the following symbols!:\n  " + context;
				break;
			case Code::ModuleNotFound:
				message = "Unable to find module '" + context + "'!";
				break;
			case Code::AmbigiousModule:
				message = "The module name '" + context + "' is ambigious!";
				break;
			default:
				message = "Unknown error code! Context: " + context;
			}

			m_whatBuff = message;
		}
	public:
		virtual explicit operator bool() const override { return m_code != Code::Success; }
	private:
		Code m_code;
	};

	typedef LinkerError::Code LinkErrCode;
}