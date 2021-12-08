#pragma once

#include <stdexcept>

namespace MarC
{
	class MarCoreError : public std::runtime_error
	{
	public:
		MarCoreError() = delete;
		MarCoreError(const char* errType, const std::string& message = "")
			: std::runtime_error(errType), m_message(message)
		{}
		virtual ~MarCoreError() noexcept = default;
	public:
		virtual const char* what() const noexcept override { return std::runtime_error::what(); }
	public:
		explicit operator bool() const { return !m_message.empty(); };
	private:
		std::string m_message;
	};
}
