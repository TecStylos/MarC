#pragma once

#include <cstdint>

namespace MarCmd
{
	template <class F>
	class Flags
	{
		uint64_t m_flags = 0;
	public:
		Flags() = default;
	public:
		bool hasFlag(F flag) const { return m_flags & iF(flag); }
		void setFlag(F flag) { m_flags |= iF(flag); }
		void clrFlag(F flag) { m_flags &= ~iF(flag); }
	private:
		static uint64_t iF(F flag) { return 1ull << (uint64_t)flag; }
	};

	enum class CmdFlags
	{
		Verbose,
		Debug,
		Profile
	};
}