#pragma once

#include <string>
#include <stdexcept>

#define DEFINE_ERROR(_x_, _base_)                  \
struct _x_ : public _base_                         \
{                                                  \
    explicit _x_(const std::string &what)          \
        : _base_(what)                             \
    {                                              \
    }                                              \
}

namespace Solution
{

DEFINE_ERROR(Exception, std::runtime_error);

DEFINE_ERROR(BrokerError, Exception);

DEFINE_ERROR(PartitionError, Exception);

}