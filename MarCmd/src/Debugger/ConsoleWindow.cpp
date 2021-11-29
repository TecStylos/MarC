#include "Debugger/ConsoleWindow.h"

#include <iostream>
#include <cstring>

#include "Debugger/ConsoleHelper.h"

namespace MarCmd
{
	namespace Console
	{
		Window::Window(const std::string& name)
			: m_name(name)
		{}

		const std::string& Window::getName() const
		{
			return m_name;
		}

		TextBufWindow::TextBufWindow(const std::string& name)
			: Window(name)
		{
			resize(1, 1);
		}

		void TextBufWindow::setPos(uint64_t newX, uint64_t newY)
		{
			m_x = newX;
			m_y = newY;
		}

		void TextBufWindow::resize(uint64_t newWidth, uint64_t newHeight)
		{
			m_width = newWidth;
			m_height = newHeight;
			m_buffer.resize(m_height);
			for (uint64_t i = 0; i < m_height; ++i)
				m_buffer[i].resize(m_width, ' ');
		}

		void TextBufWindow::writeToBuff(const char* text, uint64_t x, uint64_t y)
		{
			if (x >= m_width || y >= m_buffer.size())
				return;

			uint64_t xBackup = x;

			while (isprint(*text))
			{
				if (isprint(*text))
				{
					m_buffer[y][x++] = *text;
				}
				else
				{
					switch (*text)
					{
					case '\n':
						++y;
						x = xBackup;
						break;
					case '\t':
						writeToBuff("  ", x, y);
						x += 2;
						break;
					}
				}

				if (x >= m_width || y >= m_height)
					return;

				++text;
			}
		}

		void TextBufWindow::render(uint64_t offX, uint64_t offY) const
		{
			for (TextFormat tf : m_formats)
				std::cout << tf;

			for (uint64_t i = 0; i < m_height; ++i)
			{
				std::cout << Console::CursorPos(offY + m_y + i, offX + m_x);
				std::cout << m_buffer[i];
			}

			std::cout << TFC::F_Default;
		}

		uint64_t TextBufWindow::getWidth() const
		{
			return m_width;
		}

		uint64_t TextBufWindow::getHeight() const
		{
			return m_height;
		}

		uint64_t TextBufWindow::getX() const
		{
			return m_x;
		}

		uint64_t TextBufWindow::getY() const
		{
			return m_y;
		}

		void TextBufWindow::addTextFormat(TextFormat tf)
		{
			m_formats.push_back(tf);
		}

		void TextBufWindow::clearTextFormats()
		{
			m_formats.clear();
		}

		void TextBufWindow::clearBuffer()
		{
			for (auto& line : m_buffer)
				for (uint64_t i = 0; i < line.size(); ++i)
					line[i] = ' ';
		}

		TextWindow::TextWindow(const std::string& name)
			: TextBufWindow(name)
		{}

		void TextWindow::resize(uint64_t newWidth, uint64_t newHeight)
		{
			TextBufWindow::resize(newWidth, newHeight);
			rewrite();
		}

		void TextWindow::insert(const std::string& text, uint64_t x, uint64_t y)
		{
			while (m_text.size() < y + 1)
				m_text.push_back(std::string(x, ' '));

			auto it = m_text.begin() + y;
			uint64_t offset = 0;
			while (offset < text.size())
			{
				if (isprint(text[offset]))
				{
					it->push_back(text[offset]);
				}
				else
				{
					switch (text[offset])
					{
					case '\t':
						it->append("  ");
						break;
					case '\n':
						it = m_text.insert(++it, std::string(x, ' '));
						break;
					}
				}

				++offset;
			}

			rewrite();
		}

		void TextWindow::append(const std::string& text)
		{
			insert(text, m_text.empty() ? 0 : m_text.back().size(), m_text.empty() ? 0 : m_text.size() - 1);
		}

		void TextWindow::replace(const std::string& text, uint64_t x, uint64_t y)
		{
			while (m_text.size() < y)
				m_text.push_back(std::string());

			m_text.erase(m_text.begin() + y);
			insert(text, x, y);
		}

		bool TextWindow::wrapping() const
		{
			return m_wrapping;
		}

		void TextWindow::wrapping(bool status)
		{
			m_wrapping = status;
			rewrite();
		}

		int64_t TextWindow::getScroll() const
		{
			return m_scrollPos;
		}

		void TextWindow::setScroll(int64_t line)
		{
			m_scrollPos = line;
			rewrite();
		}

		void TextWindow::scroll(int64_t nLines)
		{
			m_scrollPos += nLines;
			rewrite();
		}

		TextWindowRef TextWindow::create(const std::string& name)
		{
			return std::shared_ptr<TextWindow>(new TextWindow(name));
		}

		void TextWindow::rewrite()
		{
			clearBuffer();

			uint64_t lineShift = 0;
			for (int64_t line = m_scrollPos; line < (int64_t)m_text.size(); ++line)
			{
				if (line < 0)
				{
					if (++lineShift >= m_height)
						break;
				}
				else
				{
					uint64_t nCharsInLine = m_text[line].size();
					const char* lineStr = m_text[line].c_str();

					uint64_t nWritten = 0;
					while (nWritten < nCharsInLine)
					{
						writeToBuff(lineStr, 0, lineShift);

						lineStr += m_width;
						nWritten += m_width;

						if (++lineShift >= m_height)
							break;

						if (!m_wrapping)
							break;
					}
				}
			}
		}

		SplitWindow::SplitWindow(const std::string& name)
			: Window(name)
		{}

		void SplitWindow::setPos(uint64_t newX, uint64_t newY)
		{
			m_x = newX;
			m_y = newY;
		}

		void SplitWindow::resize(uint64_t newWidth, uint64_t newHeight)
		{
			m_width = newWidth;
			m_height = newHeight;
			update();
		}

		void SplitWindow::render(uint64_t offX, uint64_t offY) const
		{
			if (m_wndTopLeft)
				m_wndTopLeft->render(offX + m_x, offY + m_y);
			if (m_wndBottomRight)
				m_wndBottomRight->render(offX + m_x, offY + m_y);
		}

		uint64_t SplitWindow::getWidth() const
		{
			return m_width;
		}

		uint64_t SplitWindow::getHeight() const
		{
			return m_height;
		}

		uint64_t SplitWindow::getX() const
		{
			return m_x;
		}

		uint64_t SplitWindow::getY() const
		{
			return m_y;
		}

		void SplitWindow::setRatio(WindowRatioType wrt, uint64_t ratio)
		{
			m_wrt = wrt;
			m_ratio = ratio;
			update();
		}

		WindowRef SplitWindow::getTop() const
		{
			return m_wndTopLeft;
		}

		WindowRef SplitWindow::getLeft() const
		{
			return m_wndTopLeft;
		}

		WindowRef SplitWindow::getBottom() const
		{
			return m_wndBottomRight;
		}

		WindowRef SplitWindow::getRight() const
		{
			return m_wndBottomRight;
		}

		void SplitWindow::setTop(WindowRef wndRef)
		{
			m_wndTopLeft = wndRef;
			update(wndRef);
		}

		void SplitWindow::setLeft(WindowRef wndRef)
		{
			m_wndTopLeft = wndRef;
			update(wndRef);
		}

		void SplitWindow::setBottom(WindowRef wndRef)
		{
			m_wndBottomRight = wndRef;
			update(wndRef);
		}

		void SplitWindow::setRight(WindowRef wndRef)
		{
			m_wndBottomRight = wndRef;
			update(wndRef);
		}

		SplitWindowRef SplitWindow::create(const std::string& name)
		{
			return std::shared_ptr<SplitWindow>(new SplitWindow(name));
		}

		void SplitWindow::update()
		{
			update(m_wndTopLeft);
			update(m_wndBottomRight);
		}

		void SplitWindow::update(WindowRef wndRef)
		{
			if (!wndRef)
				return;

			auto abs = calcAbsDimPos(m_width, m_height, m_wrt, m_ratio);

			if (wndRef == m_wndTopLeft)
			{
				m_wndTopLeft->setPos(abs.tlX, abs.tlY);
				m_wndTopLeft->resize(abs.tlW, abs.tlH);
			}
			if (wndRef == m_wndBottomRight)
			{
				m_wndBottomRight->setPos(abs.brX, abs.brY);
				m_wndBottomRight->resize(abs.brW, abs.brH);
			}
		}

		WndAbsDimPos SplitWindow::calcAbsDimPos(uint64_t width, uint64_t height, WindowRatioType wrt, uint64_t ratio)
		{
			WndAbsDimPos wadp = { 0 };

			switch (wrt)
			{
			case WindowRatioType::AbsoluteTop:
				ratio = std::min(ratio, height);
				calcAbsDim(wadp.tlW, wadp.brW, wadp.tlH, wadp.brH, width, height, ratio);
				wadp.brY = wadp.tlH;
				break;
			case WindowRatioType::AbsoluteRight:
				ratio = std::min(ratio, width);
				calcAbsDim(wadp.brH, wadp.tlH, wadp.brW, wadp.tlW, height, width, ratio);
				wadp.brX = wadp.tlW;
				break;
			case WindowRatioType::AbsoluteBottom:
				ratio = std::min(ratio, height);
				calcAbsDim(wadp.brW, wadp.tlW, wadp.brH, wadp.tlH, width, height, ratio);
				wadp.brY = wadp.tlH;
				break;
			case WindowRatioType::AbsoluteLeft:
				ratio = std::min(ratio, width);
				calcAbsDim(wadp.tlH, wadp.brH, wadp.tlW, wadp.brW, height, width, ratio);
				wadp.brX = wadp.tlW;
				break;
			case WindowRatioType::RelativeTop:
				wadp = calcAbsDimPos(width, height, WindowRatioType::AbsoluteTop, height * ratio / 100);
				break;
			case WindowRatioType::RelativeRight:
				wadp = calcAbsDimPos(width, height, WindowRatioType::AbsoluteRight, width * ratio / 100);
				break;
			case WindowRatioType::RelativeBottom:
				wadp = calcAbsDimPos(width, height, WindowRatioType::AbsoluteBottom, height * ratio / 100);
				break;
			case WindowRatioType::RelativeLeft:
				wadp = calcAbsDimPos(width, height, WindowRatioType::AbsoluteLeft, width * ratio / 100);
				break;
			}

			return wadp;
		}

		void SplitWindow::calcAbsDim(uint64_t& cFirst, uint64_t& cSecond, uint64_t& vFirst, uint64_t& vSecond, uint64_t cAbs, uint64_t vAbs, uint64_t rAbs)
		{
			cFirst = cAbs;
			cSecond = cAbs;
			vFirst = rAbs;
			vSecond = vAbs - rAbs;
		}

		bool subTextWndInsert(SplitWindowRef swr, const std::string& textWndName, const std::string& text, uint64_t x, uint64_t y)
		{
			auto wndConTitle = swr->getSubWndByName<TextWindow>(textWndName);
			if (wndConTitle)
				wndConTitle->insert(text, x, y);
			return !!wndConTitle;
		}
	}
}
