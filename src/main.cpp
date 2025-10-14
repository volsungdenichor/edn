#include <edn.hpp>
#include <fstream>
#include <iostream>
#include <memory>

void run(const std::vector<std::string>& args)
{
    using namespace edn::literals;
    const edn::value_t value = edn::parse("(* (+ 1 2) (+ 3 4))");

    std::cout << "expr: " << value << "\n\n";
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
