#pragma once

#include <PluS.h>

#include "types/AssemblerTypes.h"

namespace MarC
{
	struct ExFuncData
	{
		static constexpr uint64_t EXFUNC_MAX_PARAMS = 8;
		TypeCell retVal;
		uint8_t nParams = 0;
		TypeCell param[EXFUNC_MAX_PARAMS];
	};

	class ExternalFunction : public PluS::Feature
	{
	public:
		using PluS::Feature::Feature;
		virtual void call(class Interpreter& interpreter, MarC::ExFuncData& efd) = 0;
	};

	typedef ExternalFunction* ExternalFunctionPtr;
}