#pragma once

namespace MarCmd
{
	class CmdArgParser
	{
	public:
		CmdArgParser(int argc, const char** argv)
			: m_argc(argc), m_argv(argv), m_currArg(0)
		{}
	public:
		bool hasNext() const { return m_currArg + 1 < m_argc; }
		const char* getNext() { return m_argv[++m_currArg]; }
	private:
		int m_argc;
		int m_currArg;
		const char** m_argv;
	};
}