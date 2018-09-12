#pragma once

#include <cstdint> // get the feture test macros

#ifdef __cpp_lib_filesystem
	#include <filesystem>
	namespace fs = std::filesystem;
	using ifstream_t = std::ifstream;
#else
	#include <boost/filesystem.hpp>
	#include <boost/filesystem/fstream.hpp>
	namespace fs = boost::filesystem;
	using ifstream_t = fs::ifstream;
#endif