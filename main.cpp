#include <boost/process.hpp>
#include <boost/chrono.hpp>
#include <string>
#include <vector>
#include <iostream>
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
			
				try {
					count = std::stoull(*(++it));
				} catch(std::out_of_range&)
				{
					std::cout << "Number too large for --count.\n";
					return EXIT_FAILURE;
				} catch (std::invalid_argument&)
				{
					std::cout << "--count can only accept numeric values.\n";
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

	auto* in = std::fopen(in_path.c_str(), "r");
	if (!in && !in_path.empty()) {
		std::cout << "Couldn't open or find --in file.\n";
		return EXIT_FAILURE;
	}

	auto* out = std::fopen(out_path.c_str(), "w");
	if (!out && !out_path.empty()) {
		std::cout << "Couldn't open --out file for writing.\n";
		return EXIT_FAILURE;
	}

	auto* err = std::fopen(err_path.c_str(), "w");
	if(!err && !err_path.empty()) {
		std::cout << "Couldn't open --err file for writing.\n";
		return EXIT_FAILURE;
	}

	auto first = true;
	long double duration = 0;
	long double n = 1;
	
	for (unsigned long long iter = 0; iter < count; iter++)
	{
		std::string s(7, '\0');
		auto str_out = std::snprintf(&s[0], s.size(), "%.2f", (static_cast<double>(iter + 1) / static_cast<double>(count)) * 100);
		s.resize(str_out);
		std::cout << "Progress: " << iter + 1 << "/" << count << " ... " << s << "% done." << '\r' << std::flush;
		
		auto start = bench_clock::now();
		bp::child proc(bp::exe(executable_path), bp::std_in < in, bp::std_out > out, bp::std_err > out, bp::args(proc_arguments));
		proc.wait();
		auto end = bench_clock::now();
		std::rewind(in);

		if (iter == 1 && count >= 5)
			continue;

		n++;

		long double interval = static_cast<long double>(boost::chrono::duration_cast<boost::chrono::milliseconds>(end - start).count());
		if (first) {
			duration += interval;
			first = false;
		}
		else
			duration = (duration * (n - 1) + interval) / n;
	}

	if(duration <= 0.001)
		std::cout << "\nMeasured duration: " << duration*1000000 << " nanoseconds\n";
	else if(duration <= 0)
		std::cout << "\nMeasured duration: " << duration * 1000 << " microseconds\n";
	else if(duration <= 1000)
		std::cout << "\nMeasured duration: " << duration << " milliseconds\n";
	else
		std::cout << "\nMeasured duration: " << duration / 1000 << " seconds\n";
	
	return EXIT_SUCCESS;
}
