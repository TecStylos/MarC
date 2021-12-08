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
		virtual const char* what() const noexcept override { return m_whatBuff.c_str(); }
	public:
		virtual explicit operator bool() const { return true; };
	private:
		std::string m_message;
	protected:
		std::string m_whatBuff;
	};
}
