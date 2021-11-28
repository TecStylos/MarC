#include "Debugger/ConsoleWindow.h"

#include <iostream>

#include "Debugger/ConsoleHelper.h"

namespace MarCmd
{
	namespace Console
	{
		Window::Window(uint64_t width, uint64_t height)
		{
			resize(width, height);
		}

		void Window::resize(uint64_t newWidth, uint64_t newHeight)
		{
			m_width = newWidth;
			m_height = newHeight;
			m_buffer.resize(m_height);
			for (uint64_t i = 0; i < m_height; ++i)
				m_buffer[i].resize(m_width, ' ');
		}

		void Window::write(const std::string& text, uint64_t x, uint64_t y)
		{
			if (text.find('\n') != std::string::npos)
				return;
			if (x >= m_width || y >= m_buffer.size())
				return;

			uint64_t nToCpy = std::min(text.size(), m_width - x);
			memcpy((void*)(m_buffer[y].c_str() + x), text.c_str(), nToCpy);
		}

		void Window::render(uint64_t x, uint64_t y) const
		{
			for (TextFormat tf : m_formats)
				std::cout << tf;

			for (uint64_t i = 0; i < m_height; ++i)
			{
				std::cout << Console::CursorPos(y + i, x);
				std::cout << m_buffer[i];
			}

			if (m_hasForegroundModifier)
				std::cout << Console::TextFormat::FG_Default;
			if (m_hasBackgroundModifier)
				std::cout << Console::TextFormat::BG_Default;
		}

		void Window::addTextFormat(TextFormat tf)
		{
			if (Console::TextFormat::_FG_BEGIN < tf && tf < Console::TextFormat::_FG_END)
				m_hasForegroundModifier = true;
			if (Console::TextFormat::_BG_BEGIN < tf && tf < Console::TextFormat::_BG_END)
				m_hasBackgroundModifier = true;
			m_formats.push_back(tf);
		}

		void Window::clearTextFormats()
		{
			m_formats.clear();
			m_hasForegroundModifier = false;
			m_hasBackgroundModifier = false;
		}
	}
}