#pragma once

#include <list>
#include <string>
#include <vector>
#include <memory>

#include "MarCmdFlags.h"
#include "ConsoleHelper.h"

namespace MarCmd
{
	namespace Console
	{
		typedef std::shared_ptr<class Window> WindowRef;
		class Window
		{
		protected:
			Window() = delete;
			Window(const std::string& name = "<unnamed>");
			Window(Window&&) = default;
			Window(const Window&) = default;
		public:
			const std::string& getName() const;
		public:
			virtual void setPos(uint64_t newX, uint64_t newY) = 0;
			virtual void resize(uint64_t newWidth, uint64_t newHeight) = 0;
			virtual void render(uint64_t offX, uint64_t offY) const = 0;
		protected:
			std::string m_name;

		};

		typedef std::shared_ptr<class TextBufWindow> TextBufWindowRef;
		class TextBufWindow : public Window
		{
		protected:
			TextBufWindow() = delete;
			TextBufWindow(const std::string& name);
			TextBufWindow(TextBufWindow&&) = delete;
			TextBufWindow(const TextBufWindow&) = delete;
		public:
			virtual void setPos(uint64_t newX, uint64_t newY) override;
			virtual void resize(uint64_t newWidth, uint64_t newHeight) override;
			virtual void render(uint64_t offX, uint64_t offY) const override;
		public:
			void writeToBuff(const char* text, uint64_t x, uint64_t y);
		public:
			void addTextFormat(TextFormat tf);
			void clearTextFormats();
			void clearBuffer();
		public:
			static TextBufWindow create(const std::string& name);
		protected:
			uint64_t m_x = 0;
			uint64_t m_y = 0;
			uint64_t m_width;
			uint64_t m_height;
		private:
			std::vector<std::string> m_buffer;
			std::vector<TextFormat> m_formats;
		};

		typedef std::shared_ptr<class TextWindow> TextWindowRef;
		class TextWindow : public TextBufWindow
		{
		protected:
			TextWindow() = delete;
			TextWindow(const std::string& name);
			TextWindow(TextWindow&&) = delete;
			TextWindow(const TextWindow&) = delete;
		public:
			virtual void resize(uint64_t newWidth, uint64_t newHeight) override;
		public:
			void insert(const std::string& text, uint64_t x, uint64_t y);
			void append(const std::string& text);
			bool wrapping() const;
			void wrapping(bool status);
		public:
			uint64_t scrollPos() const;
			void scroll(int64_t nLines);
		public:
			static TextWindowRef create(const std::string& name);
		protected:
			void rewrite();
		private:
			int64_t m_scrollPos = 0;
			bool m_wrapping = true;
			std::vector<std::string> m_text;
		};

		typedef enum class WindowRatioType
		{
			AbsoluteTop,
			AbsoluteRight,
			AbsoluteBottom,
			AbsoluteLeft,
			RelativeTop,
			RelativeRight,
			RelativeBottom,
			RelativeLeft
		} WRT;

		struct WndAbsDimPos
		{
			uint64_t tlX, tlY, tlW, tlH;
			uint64_t brX, brY, brW, brH;
		};

		typedef std::shared_ptr<class SplitWindow> SplitWindowRef;
		class SplitWindow : public Window
		{
		protected:
			SplitWindow() = delete;
			SplitWindow(const std::string& name);
			SplitWindow(SplitWindow&&) = delete;
			SplitWindow(const SplitWindow&) = delete;
		public:
			virtual void setPos(uint64_t newX, uint64_t newY) override;
			virtual void resize(uint64_t newWidth, uint64_t newHeight) override;
			virtual void render(uint64_t offX, uint64_t offY) const override;
		public:
			void setRatio(WindowRatioType wrt, uint64_t ratio);
			WindowRef getTop() const;
			WindowRef getLeft() const;
			WindowRef getBottom() const;
			WindowRef getRight() const;
			void setTop(WindowRef wndRef);
			void setLeft(WindowRef wndRef);
			void setBottom(WindowRef wndRef);
			void setRight(WindowRef wndRef);
		public:
			template <class WindowClass>
			WindowClass* getSubWndByName(const std::string& name);
		public:
			static SplitWindowRef create(const std::string& name);
		private:
			void update();
			void update(WindowRef wndRef);
			static WndAbsDimPos calcAbsDimPos(uint64_t width, uint64_t height, WindowRatioType wrt, uint64_t ratio);
			static void calcAbsDim(uint64_t& cFirst, uint64_t& cSecond, uint64_t& vFirst, uint64_t& vSecond, uint64_t cAbs, uint64_t vAbs, uint64_t rAbs);
		protected:
			uint64_t m_x = 0;
			uint64_t m_y = 0;
			uint64_t m_width = 0;
			uint64_t m_height = 0;
			WindowRatioType m_wrt = WindowRatioType::RelativeLeft;
			uint64_t m_ratio = 50;
		private:
			WindowRef m_wndTopLeft = nullptr;
			WindowRef m_wndBottomRight = nullptr;
		};

		bool subTextWndInsert(SplitWindowRef swr, const std::string& textWndName, const std::string& text, uint64_t x, uint64_t y);

		template <class WindowClass>
		WindowClass* SplitWindow::getSubWndByName(const std::string& name)
		{
			static_assert(std::is_base_of<Window, WindowClass>::value);

			WindowClass* temp = nullptr;
			SplitWindow* subSplitWnd = nullptr;
			if (m_wndTopLeft)
			{
				if (
					(
						(temp = dynamic_cast<WindowClass*>(m_wndTopLeft.get())) != nullptr &&
						m_wndTopLeft->getName() == name
						) ||
					(
						(subSplitWnd = dynamic_cast<SplitWindow*>(m_wndTopLeft.get())) != nullptr &&
						(temp = subSplitWnd->getSubWndByName<WindowClass>(name)) != nullptr
						)
					)
				{
					return temp;
				}
			}

			if (m_wndBottomRight)
			{
				if (
					(
						(temp = dynamic_cast<WindowClass*>(m_wndBottomRight.get())) != nullptr &&
						m_wndBottomRight->getName() == name
						) ||
					(
						(subSplitWnd = dynamic_cast<SplitWindow*>(m_wndBottomRight.get())) != nullptr &&
						(temp = subSplitWnd->getSubWndByName<WindowClass>(name)) != nullptr
						)
					)
				{
					return temp;
				}
			}

			return nullptr;
		}
	}
}
