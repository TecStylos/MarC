#pragma once

#include <stdint.h>
#include <set>
#include <map>

namespace MarC
{
	template <typename T>
	uint64_t searchBinary(const T& elem, const T* first, const T* last)
	{
		const T* begin = first;
		while (first <= last)
		{
			const T* curr = first + (last - first) / 2;
			if (*curr < elem)
				first = curr + 1;
			else if (*curr > elem)
				last = curr - 1;
			else
				return uint64_t(curr - begin);
		}

		return -1;
	}

	template <typename T, class C>
	uint64_t searchBinary(const T& elem, const C& container)
	{
		return binarySearch(elem, &*container.begin(), &*(container.end() - 1));
	}

	template <typename Key, typename Val>
	const std::pair<const Key, Val>& findGreatestSmaller(const Key& elem, const std::map<Key, Val>& cont)
	{
		auto it = cont.lower_bound(elem);
		if (it != cont.begin())
			--it;
		return *it;
	}

	template <typename Key>
	const Key& findGreatestSmaller(const Key& elem, const std::set<Key>& cont)
	{
		auto it = cont.lower_bound(elem);
		if (it != cont.begin())
			--it;
		return *it;
	}
}