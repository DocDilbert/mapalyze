cmake_minimum_required(VERSION 3.10)

# set the project name
project(mapalyze_project)

find_package(Boost 1.73.0)


# add the executable
add_executable(mapalyze
	main.cc
)

set_property(TARGET mapalyze PROPERTY CXX_STANDARD 14)

target_include_directories(mapalyze PUBLIC ${Boost_INCLUDE_DIRS})

find_package(cxxopts CONFIG REQUIRED)
target_link_libraries(mapalyze PRIVATE cxxopts::cxxopts)
