#include "Debugger/ConsoleWindow.h"

#include <iostream>

#include "Debugger/ConsoleHelper.h"

namespace MarCmd
{
	namespace Console
	{
		Window::Window(uint64_t w, uint64_t h, uint64_t x, uint64_t y)
		{
			resize(w, h);
			setPos(x, y);
		}

		void Window::resize(uint64_t newWidth, uint64_t newHeight)
		{
			m_width = newWidth;
			m_height = newHeight;
			m_buffer.resize(m_height);
			for (uint64_t i = 0; i < m_height; ++i)
				m_buffer[i].resize(m_width, ' ');
		}

		void Window::setPos(uint64_t newX, uint64_t newY)
		{
			m_x = newX;
			m_y = newY;
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

		void Window::render() const
		{
			for (TextFormat tf : m_formats)
				std::cout << tf;

			for (uint64_t i = 0; i < m_height; ++i)
			{
				std::cout << Console::CursorPos(m_y + i, m_x);
				std::cout << m_buffer[i];
			}

			std::cout << TFC::F_Default;
		}

		void Window::addTextFormat(TextFormat tf)
		{
			m_formats.push_back(tf);
		}

		void Window::clearTextFormats()
		{
			m_formats.clear();
		}
	}
}