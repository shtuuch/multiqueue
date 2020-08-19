#pragma once

#include "exception.h"

#include <iostream>

namespace Solution
{
namespace
{

void printException(std::ostream &stream, const std::exception &e, int level = 0)
{
    stream << std::string(level, ' ') << "exception: " << e.what() << std::endl;
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &e) {
        printException(stream, e, level + 1);
    } catch (...) {}
}
}

class Logger
{
public:
   static std::ostream &debug()
   {
       return std::cerr;
   }
};

std::ostream &operator << (std::ostream &stream, const BrokerError &e)
{
    printException(stream, e);
    return stream;
}

}