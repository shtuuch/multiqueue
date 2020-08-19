#pragma once

#include "broker_exception.h"

#include <iostream>

namespace solution
{
namespace
{

void print_exception(std::ostream &stream, const std::exception &e, int level = 0)
{
    stream << std::string(level, ' ') << "exception: " << e.what() << std::endl;
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &e) {
        print_exception(stream, e, level + 1);
    } catch (...) {}
}
}

class logger
{
public:
   static std::ostream &debug()
   {
       return std::cerr;
   }
};

std::ostream &operator << (std::ostream &stream, const broker_error &e)
{
    print_exception(stream, e);
    return stream;
}

}