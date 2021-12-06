#pragma once

#include <stdexcept>

namespace MarC
{
	class MarCoreException : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
		virtual ~MarCoreException() noexcept = default;
	public:
		virtual const char* what() const override { return std::runtime_error::what(); }
	private:
		;
	};
}