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

		TextWindow::TextWindow(const std::string& name)
			: Window(name)
		{
			resize(1, 1);
		}

		void TextWindow::setPos(uint64_t newX, uint64_t newY)
		{
			m_x = newX;
			m_y = newY;
		}

		void TextWindow::resize(uint64_t newWidth, uint64_t newHeight)
		{
			m_width = newWidth;
			m_height = newHeight;
			m_buffer.resize(m_height);
			for (uint64_t i = 0; i < m_height; ++i)
				m_buffer[i].resize(m_width, ' ');
		}

		void TextWindow::write(const std::string& text, uint64_t x, uint64_t y)
		{
			if (x >= m_width || y >= m_buffer.size())
				return;

			uint64_t xBackup = x;

			for (char c : text)
			{
				if (isprint(c))
				{
					m_buffer[y][x++] = c;
				}
				else
				{
					switch (c)
					{
					case '\n':
						++y;
						x = xBackup;
						break;
					case '\t':
						write("  ", x, y);
						x += 2;
						break;
					}
				}

				if (x >= m_width)
				{
					if (!m_wrapping)
						return;
					++y;
					x = xBackup;
				}
				if (y >= m_height)
					return;
			}
		}

		bool TextWindow::wrapping() const
		{
			return m_wrapping;
		}

		void TextWindow::wrapping(bool status)
		{
			m_wrapping = status;
		}

		void TextWindow::render(uint64_t offX, uint64_t offY) const
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

		void TextWindow::addTextFormat(TextFormat tf)
		{
			m_formats.push_back(tf);
		}

		void TextWindow::clearTextFormats()
		{
			m_formats.clear();
		}

		TextWindowRef TextWindow::create(const std::string& name)
		{
			return std::shared_ptr<TextWindow>(new TextWindow(name));
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

		bool subTextWndWrite(SplitWindowRef swr, const std::string& textWndName, const std::string& text, uint64_t x, uint64_t y)
		{
			auto wndConTitle = swr->getSubWindowByName<TextWindow>(textWndName);
			if (wndConTitle)
				wndConTitle->write(text, x, y);
			return !!wndConTitle;
		}
	}
}
