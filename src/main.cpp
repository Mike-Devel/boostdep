#include <iostream>
#include <string>
#include <set>

#include "boostdep.h"

using namespace boost::dep;

int main(int argc, char const* argv[])
{
	if (argc < 2)
	{
		std::cout <<

			"Usage:\n"
			"\n"
			"    boostdep --list-modules\n"
			"    boostdep --list-buildable\n"
			"    boostdep [--track-sources] [--track-tests] --list-dependencies\n"
			"    boostdep --list-exceptions\n"
			"    boostdep --list-missing-headers\n"
			"\n"
			"    boostdep [options] --module-overview\n"
			"    boostdep [options] --module-levels\n"
			"    boostdep [options] --module-weights\n"
			"\n"
			"    boostdep [options] [--primary] <module>\n"
			"    boostdep [options] --secondary <module>\n"
			"    boostdep [options] --reverse <module>\n"
			"    boostdep [options] --subset <module>\n"
			"    boostdep [options] [--header] <header>\n"
			"    boostdep --test <module>\n"
			"    boostdep --cmake <module>\n"
			"    boostdep --pkgconfig <module> <version> [<var>=<value>] [<var>=<value>]...\n"
			"    boostdep [options] --subset-for <directory>\n"
			"\n"
			"    [options]: [--[no-]track-sources] [--[no-]track-tests]\n"
			"               [--html-title <title>] [--html-footer <footer>]\n"
			"               [--html-stylesheet <stylesheet>] [--html-prefix <prefix>]\n"
			"               [--html]\n";

		return -1;
	}

	for (int i = 0; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "--root") == 0)
		{
			if (i + 1 >= argc)
			{
				std::cerr << "'" << argv[i] << "': missing argument.\n";
				return -2;
			}
			fs::current_path(fs::path(argv[i + 1]));
		}
	}

	if (!find_boost_root())
	{
		std::cerr << "boostdep: Could not find Boost root.\n";
		return -2;
	}

	try
	{
		build_header_map();
	}
	catch (fs::filesystem_error const & x)
	{
		std::cout << x.what() << std::endl;
	}

	bool html = false;
	bool secondary = false;
	bool track_sources = false;
	bool track_tests = false;

	std::string html_title = "Boost Dependency Report";
	std::string html_footer;
	std::string html_stylesheet;
	std::string html_prefix;

	for (int i = 1; i < argc; ++i)
	{
		std::string option = argv[i];

		if (option == "--root")
		{
			// this option is already handled at the very beginning, 
			// so skip the option and ignore the following path argument
			i++;
		}
		else if (option == "--list-modules")
		{
			list_modules();
		}
		else if (option == "--list-buildable")
		{
			list_buildable();
		}
		else if (option == "--title" || option == "--html-title")
		{
			if (i + 1 < argc)
			{
				html_title = argv[++i];
			}
		}
		else if (option == "--footer" || option == "--html-footer")
		{
			if (i + 1 < argc)
			{
				html_footer = argv[++i];
			}
		}
		else if (option == "--html-stylesheet")
		{
			if (i + 1 < argc)
			{
				html_stylesheet = argv[++i];
			}
		}
		else if (option == "--html-prefix")
		{
			if (i + 1 < argc)
			{
				html_prefix = argv[++i];
			}
		}
		else if (option == "--html")
		{
			if (!html)
			{
				html = true;
				output_html_header(html_title, html_stylesheet, html_prefix);
			}
		}
		else if (option == "--track-sources")
		{
			track_sources = true;
		}
		else if (option == "--no-track-sources")
		{
			track_sources = false;
		}
		else if (option == "--track-tests")
		{
			track_tests = true;
		}
		else if (option == "--no-track-tests")
		{
			track_tests = false;
		}
		else if (option == "--primary")
		{
			if (i + 1 < argc)
			{
				output_module_primary_report(argv[++i], html, track_sources, track_tests);
			}
		}
		else if (option == "--secondary")
		{
			if (i + 1 < argc)
			{
				enable_secondary(secondary, track_sources, track_tests);
				output_module_secondary_report(argv[++i], html);
			}
		}
		else if (option == "--reverse")
		{
			if (i + 1 < argc)
			{
				enable_secondary(secondary, track_sources, track_tests);
				output_module_reverse_report(argv[++i], html);
			}
		}
		else if (option == "--header")
		{
			if (i + 1 < argc)
			{
				enable_secondary(secondary, track_sources, track_tests);
				output_header_report(argv[++i], html);
			}
		}
		else if (option == "--subset")
		{
			if (i + 1 < argc)
			{
				enable_secondary(secondary, track_sources, track_tests);
				output_module_subset_report(argv[++i], track_sources, track_tests, html);
			}
		}
		else if (option == "--test")
		{
			if (i + 1 < argc)
			{
				output_module_test_report(argv[++i]);
			}
		}
		else if (option == "--cmake")
		{
			if (i + 1 < argc)
			{
				output_module_cmake_report(argv[++i]);
			}
		}
		else if (option == "--module-levels")
		{
			enable_secondary(secondary, track_sources, track_tests);
			output_module_level_report(html);
		}
		else if (option == "--module-overview")
		{
			enable_secondary(secondary, track_sources, track_tests);
			output_module_overview_report(html);
		}
		else if (option == "--module-weights")
		{
			enable_secondary(secondary, track_sources, track_tests);
			output_module_weight_report(html);
		}
		else if (option == "--list-dependencies")
		{
			enable_secondary(secondary, track_sources, track_tests);
			list_dependencies();
		}
		else if (option == "--list-exceptions")
		{
			list_exceptions();
		}
		else if (option == "--list-missing-headers")
		{
			list_missing_headers();
		}
		else if (option == "--pkgconfig")
		{
			if (i + 2 < argc)
			{
				std::string module = argv[++i];
				std::string version = argv[++i];

				++i;

				output_pkgconfig(module, version, argc - i, argv + i);
			}
			else
			{
				std::cerr << "'" << option << "': missing module or version.\n";
			}

			break;
		}
		else if (option == "--subset-for")
		{
			if (i + 1 < argc)
			{
				std::string module = argv[++i];

				enable_secondary(secondary, track_sources, track_tests);

				std::set<std::string> headers;
				add_module_headers(module, headers);

				output_directory_subset_report(module, headers, html);
			}
			else
			{
				std::cerr << "'" << option << "': missing argument.\n";
			}

			break;
		}
		else if (modules().count(option))
		{
			output_module_primary_report(option, html, track_sources, track_tests);
		}
		else if (header_map().count(option))
		{
			enable_secondary(secondary, track_sources, track_tests);
			output_header_report(option, html);
		}
		else
		{
			std::cerr << "'" << option << "': not an option, module or header.\n";
		}
	}

	if (html)
	{
		output_html_footer(html_footer);
	}
}