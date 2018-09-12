#pragma once


#include "filesystem.h"


#include <string>
#include <set>
#include <map>

namespace boost {
namespace dep {

bool find_boost_root();
void build_header_map();
void list_modules();
void list_buildable();

void output_html_header(std::string const & title, std::string const & stylesheet, std::string const & prefix);
void output_html_footer(std::string const & footer);

void enable_secondary(bool & secondary, bool track_sources, bool track_tests);

void output_module_primary_report(std::string const & module, bool html, bool track_sources, bool track_tests);
void output_module_secondary_report(std::string const & module, bool html);
void output_module_reverse_report(std::string const & module, bool html);
void output_module_level_report(bool html);
void output_module_cmake_report(std::string module);
void output_module_overview_report(bool html);
void output_module_weight_report(bool html);
void output_module_subset_report(std::string const & module, bool track_sources, bool track_tests, bool html);
void output_module_test_report(std::string const & module);

void output_directory_subset_report(std::string const & module, std::set<std::string> const & headers, bool html);

void add_module_headers(fs::path const & dir, std::set<std::string> & headers);

void output_header_report(std::string const & header, bool html);

void output_pkgconfig(std::string const & module, std::string const & version, int argc, char const* argv[]);

void list_dependencies();
void list_missing_headers();
void list_exceptions();

const std::map< std::string, std::set<std::string> >& module_headers();
const std::set< std::string >& modules();
const std::map< std::string, std::string >& header_map();

}
}


