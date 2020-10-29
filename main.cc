#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <cxxopts.hpp>

int main(int argc, char* argv[]) 
{
    cxxopts::Options options("MyProgram", "One line description of MyProgram");
    options.add_options()
        ("d,debug", "Enable debugging") // a bool parameter
        ("i,integer", "Int param", cxxopts::value<int>())
        ("f,file", "File name", cxxopts::value<std::string>())
        ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Print usage")
        ;

    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
      std::cout << options.help() << std::endl;
      exit(0);
    }
    return 0;
}
