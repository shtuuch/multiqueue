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

namespace solution
{

DEFINE_ERROR(exception, std::runtime_error);

DEFINE_ERROR(broker_error, exception);

DEFINE_ERROR(partition_error, exception);

}