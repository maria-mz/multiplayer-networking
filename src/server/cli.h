#include <string>
#include <iostream>
#include <cstdlib>

inline void printHelp()
{
    std::cout <<
        "Usage: server [options]\n"
        "Options:\n"
        "  --help     Show this help message\n";
}

inline void parseArgs(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--help")
        {
            printHelp();
            std::exit(0);
        }
        else
        {
            std::cout << "Unknown argument: '" << arg << "'.\n";
            printHelp();
            std::exit(1);
        }
    }
}
