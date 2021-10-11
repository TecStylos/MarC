#include "LinkerOutput.h"

namespace MarC
{
	ExecutableInfo::ExecutableInfo()
	{
		staticStack = std::make_shared<Memory>();
	}
}