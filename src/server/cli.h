#include <string>
#include <algorithm>
#include <iostream>
#include <cstdlib>

#include "common/Utils.h"
#include "common/Constants.h"


struct CLIOptions
{
    Constants::TransportType transport = Constants::TransportType::UDP;
};

inline void printHelp()
{
    std::cout <<
        "Usage: server [options]\n"
        "Options:\n"
        "  --transport=<udp|tcp>     Transport type (default: udp)\n";
}

inline Constants::TransportType parseTransportType(const std::string& arg)
{
    if (iequals(arg, "udp"))
        return Constants::TransportType::UDP;
    if (iequals(arg, "tcp"))
        return Constants::TransportType::TCP;

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
