#pragma once

#include <cstdint>

namespace MarCmd
{
	enum class CmdFlags
	{
		Verbose,
		DebugInfo,
		Profile,
		GrantAll,
		NoExitInfo,
		ForceRefresh,
	};
}
