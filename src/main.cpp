#include <edn.hpp>
#include <fstream>
#include <iostream>
#include <memory>

void run(const std::vector<std::string>& args)
{
    edn::stack_t stack{ {} };
    std::cout << edn::evaluate(edn::parse("'(+ 2 3)"), stack) << std::endl;
}

int main(int argc, char** argv)
{
    try
    {
        run(std::vector<std::string>(argv, argv + argc));
    }
    catch (const std::exception& ex)
    {
        std::cout << "\nError:\n" << ex.what() << "\n";
    }
}
