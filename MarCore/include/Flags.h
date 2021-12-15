#pragma once

namespace MarC
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
		void clear() { m_flags = 0; }
		Flags operator<<(F flag) { setFlag(flag); return *this; }
		Flags operator>>(F flag) { clrFlag(flag); return *this; }
	private:
		static uint64_t iF(F flag) { return 1ull << (uint64_t)flag; }
	};
}