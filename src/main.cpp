#include <edn.hpp>
#include <fstream>
#include <iostream>
#include <memory>

void run(const std::vector<std::string>& args)
{
    const edn::value_t val = edn::parse("'(+ 2 3)");

    edn::stack_t stack{ {} };
    std::cout << edn::evaluate(val, stack) << std::endl;

    std::cout << val << " {" << val.type() << "}" << std::endl;
    for (const auto& v : val.vector().value())
    {
        std::cout << "  " << v << " {" << v.type() << "}" << std::endl;
    }
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
