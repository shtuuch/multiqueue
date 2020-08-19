#pragma once
#include <map>
#include <list>
#include <thread>
#include <mutex>

namespace Solution
{

template<typename Key, typename Value>
class IConsumer
{
public:
    virtual void consume(const Key &, const Value &) const = 0;

    virtual ~IConsumer()
    {};
};

}