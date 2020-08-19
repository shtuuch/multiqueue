#pragma once
#include <map>
#include <list>
#include <thread>
#include <mutex>

namespace solution
{

template<typename Key, typename Value>
class consumer
{
public:
    virtual void consume(const Key &, const Value &) const = 0;

    virtual ~consumer()
    {};
};

}