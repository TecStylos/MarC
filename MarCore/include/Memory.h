#pragma once

#include <vector>
#include <stdint.h>

namespace MarC
{
	class Memory
	{
	public:
		Memory();
		Memory(uint64_t initSize, bool resizable = true);
	public:
		template <typename T>
		bool read(T& data, uint64_t offset);
		template <typename T>
		bool write(const T& data, uint64_t offset);
		template <typename T>
		bool push(const T& data);
	public:
		bool read(void* data, uint64_t size, uint64_t offset);
		bool write(const void* data, uint64_t size, uint64_t offset);
		bool push(const void* data, uint64_t size);
	public:
		bool resize(uint64_t newSize);
		bool autoResize(uint64_t newSize);
		bool resizable() const;
		void resizable(bool state);
	public:
		void* getBaseAddress();
		uint64_t size() const;
	private:
		std::vector<char> m_data;
		bool m_resizable;
	};

	template <typename T>
	bool Memory::read(T& data, uint64_t offset)
	{
		return read(&data, sizeof(T), offset);
	}

	template <typename T>
	bool Memory::write(const T& data, uint64_t offset)
	{
		return write(&data, sizeof(T), offset);
	}

	template <typename T>
	bool Memory::push(const T& data)
	{
		return push(&data, sizeof(T));
	}
}