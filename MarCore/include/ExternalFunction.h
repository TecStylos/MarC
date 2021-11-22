#pragma once

#include <PluS.h>

#include "AssemblerTypes.h"

namespace MarC
{
	struct ExFuncData
	{
		TypeCell retVal;
		uint8_t nParams = 0;
		TypeCell param[8];
	};

	class ExternalFunction : public PluS::Feature
	{
	public:
		using PluS::Feature::Feature;
		virtual void call(class Interpreter& interpreter, MarC::ExFuncData& efd) = 0;
	};

	typedef ExternalFunction* ExternalFunctionPtr;
}