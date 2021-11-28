#pragma once

#include <string>
#include <vector>

#include "ConsoleHelper.h"

namespace MarCmd
{
	namespace Console
	{
		class Window
		{
		public:
			Window() = delete;
			Window(uint64_t width, uint64_t height);
			Window(Window&&) = delete;
			Window(const Window&) = delete;
		public:
			void resize(uint64_t newWidth, uint64_t newHeight);
			void write(const std::string& text, uint64_t x, uint64_t y);
			void render(uint64_t x, uint64_t y) const;
		public:
			void addTextFormat(TextFormat tf);
			void clearTextFormats();
		private:
			uint64_t m_width;
			uint64_t m_height;
			std::vector<std::string> m_buffer;
			std::vector<TextFormat> m_formats;
			bool m_hasForegroundModifier = false;
			bool m_hasBackgroundModifier = false;
		};
	}
}