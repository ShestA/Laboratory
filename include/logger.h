// Copyright 2020 <DreamTeamGo>

#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup.hpp>

using namespace boost::log::trivial;

class Logger
{
public:
	static void init(const std::string& str)
	{
		const std::string log_name = str.empty() ? "/tmp/logFile" : str;
		boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");
		init_logger(log_name);
		boost::log::add_common_attributes();
	}

private:

	static void init_logger(const std::string& str)
	{
		auto log = boost::log::add_file_log(
			boost::log::keywords::file_name = str + "_%N.log",
			boost::log::keywords::rotation_size = 10 * 1024 * 1024,
			boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point{ 0, 0, 0 },
			boost::log::keywords::format = "[%TimeStamp%]: %Message%"
		);

		auto log2 = boost::log::add_console_log(
			std::cout,
			boost::log::keywords::format = "[%TimeStamp%]: %Message%"
		);

		log->set_filter(
			boost::log::trivial::severity >= boost::log::trivial::trace
		);
		log2->set_filter(
			boost::log::trivial::severity >= boost::log::trivial::info
		);
	}
};