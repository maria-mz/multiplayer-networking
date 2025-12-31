#include <string>
#include <algorithm>
#include <iostream>
#include <cstdlib>

#include "common/utils.h"
#include "common/constants.h"


struct CLIOptions
{
    constants::TransportType transport = constants::TransportType::UDP;
};

inline void printHelp()
{
    std::cout <<
        "Usage: server [options]\n"
        "Options:\n"
        "  --transport=<udp|tcp>     Transport type (default: udp)\n";
}

inline constants::TransportType parseTransportType(const std::string& arg)
{
    if (iequals(arg, "udp"))
        return constants::TransportType::UDP;
    if (iequals(arg, "tcp"))
        return constants::TransportType::TCP;

    std::cout << "Unknown transport type: '" << arg << "'.\n";
    printHelp();
    std::exit(1);
}

inline CLIOptions parseArgs(int argc, char* argv[])
{
    CLIOptions opts;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--help")
        {
            printHelp();
            std::exit(0);
        }
        else if (arg.starts_with("--transport="))
        {
            opts.transport = parseTransportType(
                arg.substr(std::string("--transport=").size())
            );
        }
        else
        {
            std::cout << "Unknown argument: '" << arg << "'.\n";
            printHelp();
            std::exit(1);
        }
    }

    return opts;
}
