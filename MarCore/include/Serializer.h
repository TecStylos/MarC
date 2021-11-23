#pragma once

#include <istream>
#include <ostream>
#include <set>
#include <vector>

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

	MARC_SERIALIZER_ENABLE_FIXED(int8_t);
	MARC_SERIALIZER_ENABLE_FIXED(int16_t);
	MARC_SERIALIZER_ENABLE_FIXED(int32_t);
	MARC_SERIALIZER_ENABLE_FIXED(int64_t);
	MARC_SERIALIZER_ENABLE_FIXED(uint8_t);
	MARC_SERIALIZER_ENABLE_FIXED(uint16_t);
	MARC_SERIALIZER_ENABLE_FIXED(uint32_t);
	MARC_SERIALIZER_ENABLE_FIXED(uint64_t);

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

	template <>
	inline void serialize(const std::vector<std::string>& strVec, std::ostream& oStream)
	{
		serialize<uint64_t>(strVec.size(), oStream);
		for (auto& reqMod : strVec)
			serialize(reqMod, oStream);
	}
	template <>
	inline void deserialize(std::vector<std::string>& strVec, std::istream& iStream)
	{
		strVec.clear();
		uint64_t nElements;
		deserialize(nElements, iStream);
		strVec.resize(nElements);
		for (uint64_t i = 0; i < nElements; ++i)
			deserialize(strVec[i], iStream);
	}

	template <>
	inline void serialize(const std::set<std::string>& strSet, std::ostream& oStream)
	{
		serialize<uint64_t>(strSet.size(), oStream);
		for (auto& reqMod : strSet)
			serialize(reqMod, oStream);
	}
	template <>
	inline void deserialize(std::set<std::string>& strSet, std::istream& iStream)
	{
		strSet.clear();
		uint64_t nElements;
		deserialize(nElements, iStream);
		for (uint64_t i = 0; i < nElements; ++i)
		{
			std::string temp;
			deserialize(temp, iStream);
			strSet.insert(temp);
		}
	}
}