#include "Memory.h"

#include <cstring>

namespace MarC
{
	Memory::Memory()
		: m_resizable(true)
	{}

	Memory::Memory(uint64_t initSize, bool resizable)
		: Memory()
	{
		resize(initSize);
		Memory::resizable(resizable);
	}

	bool Memory::read(void* data, uint64_t size, uint64_t offset)
	{
		if (!autoResize(offset + size))
			return false;

		memcpy(data, m_data.data() + offset, size);

		return true;
	}

	bool Memory::write(const void* data, uint64_t size, uint64_t offset)
	{
		if (!autoResize(offset + size))
			return false;

		memcpy(m_data.data() + offset, data, size);

		return true;
	}

	bool Memory::push(const void* data, uint64_t size)
	{
		if (!autoResize(Memory::size() + size))
			return false;

		memcpy(m_data.data() + Memory::size() - size, data, size);

		return true;
	}

	bool Memory::resize(uint64_t newSize)
	{
		if (!m_resizable)
			return false;

		uint64_t newSizePadded = (newSize + 7) / 8 * 8;
		m_data.reserve(newSizePadded);
		m_data.resize(newSize);
		return true;
	}

	bool Memory::autoResize(uint64_t newSize)
	{
		if (newSize < m_data.size())
			return true;

		return resize(newSize);
	}

	bool Memory::resizable() const
	{
		return m_resizable;
	}

	void Memory::resizable(bool state)
	{
		m_resizable = state;
	}

	void* Memory::getBaseAddress()
	{
		return m_data.data();
	}

	const void* Memory::getBaseAddress() const
	{
		return m_data.data();
	}

	uint64_t Memory::size() const
	{
		return m_data.size();
	}

	MemoryRef Memory::create()
	{
		return std::make_shared<Memory>();
	}

	MemoryRef Memory::create(uint64_t initSize, bool resizable)
	{
		return std::make_shared<Memory>(initSize, resizable);
	}
}
