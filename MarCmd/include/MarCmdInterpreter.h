#pragma once

#include <chrono>
#include <string>

#include "Linker.h"
#include "MarCmdSettings.h"

namespace MarCmd
{
	class Interpreter
	{
	public:
		static int run(const Settings& settings);
	private:
		static std::string readFile(const std::string& filepath);
	private:
		class Timer
		{
		public:
			void start() { m_start = std::chrono::high_resolution_clock::now(); }
			void stop() { m_stop = std::chrono::high_resolution_clock::now(); }
			uint64_t microseconds() { return std::chrono::duration_cast<std::chrono::microseconds>(m_stop - m_start).count(); }
		private:
			std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
			std::chrono::time_point<std::chrono::high_resolution_clock> m_stop;
		};
	};
}