#pragma once

#include <PluS.h>

namespace MarC
{
	class ExternalFunction : public PluS::Feature
	{
	public:
		using PluS::Feature::Feature;
		virtual void call() = 0;
	};

	typedef ExternalFunction* ExternalFunctionPtr;
}