#pragma once

#include <vector>
#include <stdint.h>
#include <memory>

#include "Serializer.h"

namespace MarC
{
	class Memory;
	typedef std::shared_ptr<Memory> MemoryRef;

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
		const void* getBaseAddress() const;
		uint64_t size() const;
	public:
		static MemoryRef create();
		static MemoryRef create(uint64_t initSize, bool resizable = true);
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

	template <>
	inline void serialize(const Memory& mem, std::ostream& oStream)
	{
		serialize<uint64_t>(mem.size(), oStream);

		oStream.write((const char*)mem.getBaseAddress(), mem.size());
	}

	template <>
	inline void deserialize(Memory& mem, std::istream& iStream)
	{
		uint64_t memSize;
		deserialize(memSize, iStream);

		mem.resize(memSize);
		iStream.read((char*)mem.getBaseAddress(), mem.size());
	}
}