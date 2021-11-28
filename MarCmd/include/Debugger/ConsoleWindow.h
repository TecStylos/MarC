#pragma once

#include <string>
#include <vector>

#include "MarCmdFlags.h"
#include "ConsoleHelper.h"

namespace MarCmd
{
	namespace Console
	{
		class Window
		{
		public:
			Window() = delete;
			Window(uint64_t w, uint64_t h, uint64_t x, uint64_t y);
			Window(Window&&) = delete;
			Window(const Window&) = delete;
		public:
			void resize(uint64_t newWidth, uint64_t newHeight);
			void setPos(uint64_t newX, uint64_t newY);
			void write(const std::string& text, uint64_t x, uint64_t y);
			void render() const;
		public:
			void addTextFormat(TextFormat tf);
			void clearTextFormats();
		private:
			uint64_t m_width;
			uint64_t m_height;
			uint64_t m_x;
			uint64_t m_y;
			std::vector<std::string> m_buffer;
			std::vector<TextFormat> m_formats;
		};
	}
}