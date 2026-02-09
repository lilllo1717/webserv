#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>

std::string	readFile(const std::string& iPath, const std::string& oPath)
{
    std::ifstream inputFile(iPath.c_str());
    if (!inputFile)
        throw std::runtime_error("Error opening input file");

    std::ofstream outputFile(oPath.c_str());
    if (!outputFile)
        throw std::runtime_error("Error opening output file");

    std::string content;
    std::string line;
    while (std::getline(inputFile, line))
	{
        content += line;
        content += '\n';
        outputFile << line << std::endl;
    }

    return content;
}

#include <iostream>

int main()
{
    try
	{
        std::string text = readFile("testConfig.conf", "copy.conf");
        std::cout << "File copied successfully" << std::endl;
    } 
	catch (const std::exception& e)
	{
        std::cerr << e.what() << std::endl;
    }
}



