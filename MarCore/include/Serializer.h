#pragma once

#include <istream>
#include <ostream>

#define MARC_SERIALIZER_ENABLE_FIXED(Type) \
template <> \
inline void serialize(const Type& obj, ::std::ostream& oStream) \
{ serializeStaticSized(obj, oStream); } \
template <> \
inline void deserialize(Type& obj, ::std::istream& iStream) \
{ deserializeStaticSized(obj, iStream); }

namespace MarC
{
	template <typename T>
	void serializeStaticSized(const T& obj, std::ostream& oStream)
	{
		oStream.write((const char*)&obj, sizeof(T));
	}
	template <typename T>
	void deserializeStaticSized(T& obj, std::istream& iStream)
	{
		iStream.read((char*)&obj, sizeof(T));
	}

	template <typename T>
	void serialize(const T& obj, std::ostream& oStream) = delete;
	template <typename T>
	void deserialize(T& obj, std::istream& iStream) = delete;

	template <>
	inline void serialize(const std::string& string, std::ostream& oStream)
	{
		oStream.write(string.c_str(), string.size() + 1);
	}
	template <>
	inline void deserialize(std::string& string, std::istream& iStream)
	{
		string.clear();
		char ch;
		while ((ch = iStream.get()) != '\0')
			string.push_back(ch);
	}

	MARC_SERIALIZER_ENABLE_FIXED(int8_t);
	MARC_SERIALIZER_ENABLE_FIXED(int16_t);
	MARC_SERIALIZER_ENABLE_FIXED(int32_t);
	MARC_SERIALIZER_ENABLE_FIXED(int64_t);
	MARC_SERIALIZER_ENABLE_FIXED(uint8_t);
	MARC_SERIALIZER_ENABLE_FIXED(uint16_t);
	MARC_SERIALIZER_ENABLE_FIXED(uint32_t);
	MARC_SERIALIZER_ENABLE_FIXED(uint64_t);
}