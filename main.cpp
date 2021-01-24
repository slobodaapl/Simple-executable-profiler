#include <boost/process.hpp>
#include <boost/chrono.hpp>
#include <exception>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <cstdio>

typedef boost::chrono::steady_clock bench_clock;

// Defined map of acceptable arguments to integer for switch
const static std::unordered_map<std::string, int> string_map{
	{"--count", 1},
	{"--in", 2},
	{"--out", 2},
	{"--err", 2},
	{"process", 3}
};

// Return code of argument, or 0 if it's not defined
int string_case(std::string& s_case)
{
	return string_map.count(s_case) ? string_map.at(s_case) : 0;
}

// Check if string is numeric
bool is_digits(const std::string& str)
{
	return str.find_first_not_of("0123456789") == std::string::npos;
}

int main(int argc, char* argv[])
{
	namespace bp = boost::process;

	if(argc < 2)
	{
		std::cout << "No runnable executable specified.\n";
		return EXIT_FAILURE;
	}

	std::vector<std::string> arguments(argv + 1, argv + argc);
	auto it = std::begin(arguments);

	const auto executable_path = *it;

	std::vector<std::string> proc_arguments;
	unsigned long long count = 1;
	std::string string_num;
	std::string in_path; // Default initialized to ""
	std::string out_path;
	std::string err_path;
	
	for(const auto end = std::end(arguments); it+1 != end;)
	{
		switch(string_case(*(++it)))
		{
			default:
				std::cout << "Unexpected or illegal argument encountered: " << *it << "\n";
				return EXIT_FAILURE;
			
			case 1:
				if (it + 1 == end)
				{
					std::cout << "Unexpected end of argument list.\n";
					return EXIT_FAILURE;
				}

				string_num = *(++it);
			
				if(!is_digits(string_num))
				{
					std::cout << "Argument for --count is either negative or contains non-numeric characters.\n";
					return EXIT_FAILURE;
				}
			
				try {
					count = std::stoull(string_num);
				} catch(std::out_of_range&)
				{
					std::cout << "Number too large for --count.\n";
					return EXIT_FAILURE;
				} catch (std::invalid_argument&)
				{
					std::cout << "--count can only accept numeric values.\n";
					return EXIT_FAILURE;
				}

				if(count == 0)
				{
					std::cout << "Count must be larger than 0.\n";
					return EXIT_FAILURE;
				}
			
				break;
			
			case 2:
				if (it + 1 == end)
				{
					std::cout << "Unexpected end of argument list.\n";
					return EXIT_FAILURE;
				}

				if (*it == "--in")
					in_path = *(++it);
				else if (*it == "--out")
					out_path = *(++it);
				else if (*it == "--err")
					err_path = *(++it);

				break;

			case 3:
				while(it + 1 != end)
				{
					proc_arguments.push_back(*(++it));
				}
			
				break;
		}
	}

	auto out_stream = out_path.empty() ? (bp::std_out > stdout) : (bp::std_out > out_path);
	auto err_stream = out_path.empty() ? (bp::std_err > stderr) : (bp::std_err > err_path);

	if (!out_path.empty()) {
		std::ofstream ofs_out(out_path);
		if (!ofs_out.is_open())
		{
			std::cout << "STD_OUT file cannot be opened for writing.\n";
			return EXIT_FAILURE;
		}
		ofs_out.close();
	}

	if (!err_path.empty()) {
		std::ofstream ofs_err(err_path);
		if (!ofs_err.is_open())
		{
			std::cout << "STD_ERR file cannot be opened for writing.\n";
			return EXIT_FAILURE;
		}
		ofs_err.close();
	}

	if (!in_path.empty()) {
		std::ifstream ifs_in(in_path);
		if (!ifs_in.is_open())
		{
			std::cout << "STD_IN file cannot be opened for writing.\n";
			return EXIT_FAILURE;
		}
		ifs_in.close();
	}

	auto first = true;
	long double duration = 0;
	long double n = 1;
	int exit_code = 0;
	
	for (unsigned long long iter = 0; iter < count; iter++)
	{
		if (!in_path.empty() && !out_path.empty() && !err_path.empty() && count > 1) {
			std::string s(7, '\0');
			auto str_out = std::snprintf(&s[0], s.size(), "%.2f", (static_cast<double>(iter + 1) / static_cast<double>(count)) * 100);
			s.resize(str_out);
			std::cout << "Progress: " << iter + 1 << "/" << count << " ... " << s << "% done." << '\r' << std::flush;
		}
		
		std::unique_ptr<bp::child> proc;
		boost::chrono::time_point<boost::chrono::steady_clock> start;
		boost::chrono::time_point<boost::chrono::steady_clock> end;

		if(in_path.empty())
		{
			std::cout << "Child process may be awaiting input from stdin:\n";
		}
		
		try 
		{
			if (in_path.empty()) {
				start = bench_clock::now();
				proc = std::make_unique<bp::child>(bp::exe(executable_path), bp::std_in < stdin, out_stream, err_stream, bp::args(proc_arguments));
			}
			else {
				start = bench_clock::now();
				proc = std::make_unique<bp::child>(bp::exe(executable_path), bp::std_in < in_path, out_stream, err_stream, bp::args(proc_arguments));
			}
			(*proc).wait();
			end = bench_clock::now();
			exit_code = proc->exit_code();
			
		}
		catch (std::exception const& e)
		{
			std::cout << e.what();
			return EXIT_FAILURE;
		}

		if(count == 1)
		{
			std::cout << "Program completed with exit code " << exit_code << "\n";
		}

		if (iter == 1 && count >= 5)
			continue;

		n++;

		auto interval = static_cast<long double>(boost::chrono::duration_cast<boost::chrono::microseconds>(end - start).count());
		if (first) {
			duration += interval;
			first = false;
		}
		else
			duration = (duration * (n - 1) + interval) / n;
	}

	if(exit_code != 0)
	{
		std::cout << "Warning, program may have crashed or thrown an exception mid run, profiling may be inaccurate.\n";
	}

	if(duration <= 0)
		std::cout << "\nMeasured duration: " << duration*1000 << " nanoseconds\n";
	else if(duration <= 1000)
		std::cout << "\nMeasured duration: " << duration << " microseconds\n";
	else if(duration <= 1000000)
		std::cout << "\nMeasured duration: " << duration / 1000 << " milliseconds\n";
	else
		std::cout << "\nMeasured duration: " << duration / 1000000 << " seconds\n";
	
	return EXIT_SUCCESS;
}
