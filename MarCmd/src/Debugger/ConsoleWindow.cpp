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

		void Window::setParent(WindowWeakRef wndParent)
		{
			m_wndParent = wndParent;
		}

		WindowRef Window::getParentRef() const
		{
			return m_wndParent.lock();
		}

		void Window::setSelfRef(WindowWeakRef selfRef)
		{
			m_selfRef = selfRef;
		}

		void Window::handleKeyPress(char key)
		{
			WindowRef wndParent = m_wndParent.lock();
			if (wndParent)
				wndParent->handleKeyPress(key);
		}

		WindowRef Window::getSubWndRef(const std::string& name)
		{
			return nullptr;
		}

		bool Window::replaceSubWnd(const std::string& name, WindowRef newWnd)
		{
			return false;
		}

		BaseWindow::BaseWindow(WindowRef wndChild)
			: m_wndChild(wndChild)
		{}

		bool BaseWindow::setFocus(const std::string& name)
		{
			if (!m_wndChild)
				return false;

			WindowRef temp = m_wndChild->getSubWndRef(name);
			if (!temp)
				return false;

			m_wndFocus = temp;
			return true;
		}

		WindowRef BaseWindow::getFocus() const
		{
			return m_wndFocus.lock();
		}

		bool BaseWindow::handleKeyPress(char key)
		{
			auto focus = getFocus();
			if (!focus)
				return false;

			focus->handleKeyPress(key);
			return true;
		}

		WindowRef BaseWindow::getChild()
		{
			return m_wndChild;
		}

		WindowRef BaseWindow::operator->()
		{
			return m_wndChild;
		}

		WindowRef BaseWindow::operator*()
		{
			return m_wndChild;
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
				m_text.push_back(std::string());

			bool isNewIt = true;
			bool isFirstIt = true;
			auto it = m_text.begin() + y;
			uint64_t offset = 0;
			std::string lineBackup;
			while (offset < text.size())
			{
				if (isNewIt)
				{
					isNewIt = false;
					if (isFirstIt)
					{
						isFirstIt = false;
						if (it->size() < x)
							it->resize(x, ' ');
						lineBackup = it->substr(x);
					}
				}
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
						isNewIt = true;
						break;
					}
				}

				++offset;
			}

			if (!lineBackup.empty())
				it->append(lineBackup);

			rewrite();
		}

		void TextWindow::append(const std::string& text)
		{
			insert(text, m_text.empty() ? 0 : m_text.back().size(), m_text.empty() ? 0 : m_text.size() - 1);
		}

		void TextWindow::replace(const std::string& text, uint64_t x, uint64_t y)
		{
			while (m_text.size() < y + 1)
				m_text.push_back(std::string());

			m_text[y].clear();
			insert(text, x, y);
		}

		void TextWindow::clearText()
		{
			m_text.clear();
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

		uint64_t TextWindow::nLines() const
		{
			return m_text.size();
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
			auto temp = std::shared_ptr<TextWindow>(new TextWindow(name));
			temp->setSelfRef(temp);
			return temp;
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
					if (nCharsInLine == 0)
					{
						if (++lineShift >= m_height)
							break;
						continue;
					}
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

		WindowRef SplitWindow::getSubWndRef(const std::string& name)
		{
			WindowRef temp = nullptr;
			if (m_wndTopLeft)
			{
				if (m_wndTopLeft->getName() == name)
					return m_wndTopLeft;
				temp = m_wndTopLeft->getSubWndRef(name);
			}
			if (!temp && m_wndBottomRight)
			{
				if (m_wndBottomRight->getName() == name)
					return m_wndBottomRight;
				temp = m_wndBottomRight->getSubWndRef(name);
			}
			return temp;
		}

		bool SplitWindow::replaceSubWnd(const std::string& name, WindowRef newWnd)
		{
			if (m_wndTopLeft)
			{
				if (m_wndTopLeft->getName() == name)
				{
					setTop(newWnd);
					return true;
				}
				if (m_wndTopLeft->replaceSubWnd(name, newWnd))
					return true;
			}
			if (m_wndBottomRight)
			{
				if (m_wndBottomRight->getName() == name)
				{
					setBottom(newWnd);
					return true;
				}
				if (m_wndBottomRight->replaceSubWnd(name, newWnd))
					return true;
			}

			return false;
		}

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
			return getTop();
		}

		WindowRef SplitWindow::getBottom() const
		{
			return m_wndBottomRight;
		}

		WindowRef SplitWindow::getRight() const
		{
			return getBottom();
		}

		void SplitWindow::setTop(WindowRef wndRef)
		{
			m_wndTopLeft = wndRef;
			wndRef->setParent(m_selfRef);
			update(wndRef);
		}

		void SplitWindow::setLeft(WindowRef wndRef)
		{
			setTop(wndRef);
		}

		void SplitWindow::setBottom(WindowRef wndRef)
		{
			m_wndBottomRight = wndRef;
			wndRef->setParent(m_selfRef);
			update(wndRef);
		}

		void SplitWindow::setRight(WindowRef wndRef)
		{
			setBottom(wndRef);
		}

		SplitWindowRef SplitWindow::create(const std::string& name)
		{
			auto temp = std::shared_ptr<SplitWindow>(new SplitWindow(name));
			temp->setSelfRef(temp);
			return temp;
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

		bool subTextWndInsert(WindowRef wnd, const std::string& textWndName, const std::string& text, uint64_t x, uint64_t y)
		{
			auto wndText = wnd->getSubWnd<TextWindow>(textWndName);

			if (wndText)
				wndText->insert(text, x, y);
			return !!wndText;
		}
	}
}
