#include "ExecutableInfo.h"

namespace MarC
{
	ExecutableInfoRef ExecutableInfo::create()
	{
		return std::make_shared<ExecutableInfo>();
	}
}