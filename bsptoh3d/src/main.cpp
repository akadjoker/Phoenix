

#include "Core.hpp"
#include <iostream>
#include "bsp.hpp"

#include <iostream>

void PrintUsage(const char* programName)
{
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║          Quake exporter Tool v1.0.0                        ║" << std::endl;
    std::cout << "║          Copyright (c) 2025 Luis Santos (aka djoker)       ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << programName << " <input> <output> " << std::endl;
    std::cout << std::endl;
 

}

void PrintVersion()
{
    std::cout << "Quake BSP3 To h3d Tool v1.0.0" << std::endl;
    std::cout << "Copyright (c) 2025 Luis Santos (aka djoker)" << std::endl;
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << std::endl;
}

int main(int argc, char** argv)
{
    // if (argc < 2)
    // {
    //     PrintUsage(argv[0]);
    //     return 1;
    // }
    
    // // Check version flag
    // if (std::string(argv[1]) == "--version")
    // {
    //     PrintVersion();
    //     return 0;
    // }
    
    // if (argc < 3)
    // {
    //     PrintUsage(argv[0]);
    //     return 1;
    // }
    
    // std::string inputFile = argv[1];
    // std::string outputFile = argv[2];

    // BSP bsp;
    // bsp.loadFromFile(inputFile);
    // bsp.saveToFile(outputFile);

 

     BSP bsp;
    bsp.loadFromFile("/media/djoker/code/projects/cpp/Phoenix/bin/oa_rpg3dm2.bsp");
    bsp.saveToFile("/media/djoker/code/projects/cpp/Phoenix/bin/oa_rpg3dm2.h3d");

 

    return 0;
}
