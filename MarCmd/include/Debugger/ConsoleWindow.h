#pragma once

#include <string>
#include <vector>

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
			Window() = default;
			Window(Window&&) = default;
			Window(const Window&) = default;
		public:
			void setPos(uint64_t newX, uint64_t newY);
		public:
			virtual void resize(uint64_t newWidth, uint64_t newHeight) = 0;
			virtual void render(uint64_t offX, uint64_t offY) const = 0;
		protected:
			uint64_t m_x = 0;
			uint64_t m_y = 0;
		};

		typedef std::shared_ptr<class TextWindow> TextWindowRef;
		class TextWindow : public Window
		{
		protected:
			TextWindow();
			TextWindow(TextWindow&&) = delete;
			TextWindow(const TextWindow&) = delete;
		public:
			virtual void resize(uint64_t newWidth, uint64_t newHeight) override;
			virtual void render(uint64_t offX, uint64_t offY) const override;
		public:
			void write(const std::string& text, uint64_t x, uint64_t y);
		public:
			void addTextFormat(TextFormat tf);
			void clearTextFormats();
		public:
			static TextWindowRef create();
		protected:
			uint64_t m_width;
			uint64_t m_height;
		private:
			std::vector<std::string> m_buffer;
			std::vector<TextFormat> m_formats;
		};

		enum class WindowRatioType
		{
			AbsoluteTop,
			AbsoluteRight,
			AbsoluteBottom,
			AbsoluteLeft,
			RelativeTop,
			RelativeRight,
			RelativeBottom,
			RelativeLeft
		};

		struct WndAbsDimPos
		{
			uint64_t tlX, tlY, tlW, tlH;
			uint64_t brX, brY, brW, brH;
		};

		typedef std::shared_ptr<class SplitWindow> SplitWindowRef;
		class SplitWindow : public Window
		{
		protected:
			SplitWindow() = default;
			SplitWindow(SplitWindow&&) = delete;
			SplitWindow(const SplitWindow&) = delete;
		public:
			virtual void resize(uint64_t newWidth, uint64_t newHeight) override;
			virtual void render(uint64_t offX, uint64_t offY) const override;
		public:
			void setRatio(WindowRatioType wrt, uint64_t ratio);
			WindowRef getTopLeft() const;
			WindowRef getBottomRight() const;
			void setTopLeft(WindowRef wndRef);
			void setBottomRight(WindowRef wndRef);
		public:
			static SplitWindowRef create();
		private:
			void update();
			void update(WindowRef wndRef);
			static WndAbsDimPos calcAbsDimPos(uint64_t width, uint64_t height, WindowRatioType wrt, uint64_t ratio);
			static void calcAbsDim(uint64_t& cFirst, uint64_t& cSecond, uint64_t& vFirst, uint64_t& vSecond, uint64_t cAbs, uint64_t vAbs, uint64_t rAbs);
		protected:
			uint64_t m_width = 0;
			uint64_t m_height = 0;
			WindowRatioType m_wrt = WindowRatioType::RelativeLeft;
			uint64_t m_ratio = 50;
		private:
			WindowRef m_wndTopLeft = nullptr;
			WindowRef m_wndBottomRight = nullptr;
		};
	}
}