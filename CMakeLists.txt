cmake_minimum_required(VERSION 3.1)

project( boostdep LANGUAGES CXX )

set(CMAKE_CXX_STANDARD 17)

add_executable( boostdep src/main.cpp src/boostdep.cpp )

#find_package( Boost COMPONENTS filesystem )
#target_link_libraries( boostdep Boost::filesystem )

