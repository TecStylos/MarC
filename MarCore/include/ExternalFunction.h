#pragma once

#include <PluS.h>

namespace MarC
{
	class ExternalFunction : public PluS::Feature
	{
	public:
		virtual void call() = 0;
	};

	typedef ExternalFunction* ExternalFunctionPtr;
}