#pragma once

#include <chrono>
#include <string>

#include "Linker.h"

namespace MarCmd
{
	struct Interpreter
	{
		static int run(const std::string& inFile, const std::set<std::string>& modDirs);
	private:
		static std::string modNameFromPath(const std::string& filepath);
		static std::string readFile(const std::string& filepath);
		static bool addModule(MarC::Linker& linker, const std::string& modPath, const std::string& modName);
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