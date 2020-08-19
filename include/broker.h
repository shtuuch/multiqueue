#pragma once

#include "broker_exception.h"
#include "partition_manager.h"
#include "consumer.h"
#include "logger.h"

#include <condition_variable>
#include <functional>
#include <sstream>
#include <set>
#include <queue>
#include <mutex>
#include <atomic>

namespace solution
{

template <typename Key, typename Value>
class broker
{
using consumer_type = consumer<Key, Value>;
using consumer_collection = std::set<const consumer_type *>;

public:
    broker(size_t max_size);

    ~broker();

	template <typename ...Args>
	void push(const Key &key, Args && ...args);

	void subscribe(const Key &key, const consumer_type &consumer);

	void unsubscribe(const Key &key, const consumer_type &consumer);

private:
	void thread_proc();

    partition_manager<Key, Value> _partition_manager;
    std::atomic_bool _terminating;
    std::thread _worker_thread;
};

template<typename Key, typename Value>
broker<Key, Value>::broker(size_t max_size)
    : _partition_manager(max_size)
    , _terminating(false)
    , _worker_thread(std::bind(&broker::thread_proc, this))
{}

template<typename Key, typename Value>
broker<Key, Value>::~broker()
{
    _terminating = true;
    _partition_manager.cancel_wait();
    _worker_thread.join();
}

template<typename Key, typename Value>
template<typename... Args>
void broker<Key, Value>::push(const Key &key, Args &&... args)
{
    _partition_manager.push(key, std::forward<Args>(args)...);
}

template<typename Key, typename Value>
void broker<Key, Value>::subscribe(const Key &key, const broker::consumer_type &consumer)
{
    try {
        _partition_manager.subscribe(key, consumer);
    }
    catch (const broker_error &) {
        std::throw_with_nested(broker_error("Failed to subscribe"));
    }
}

template<typename Key, typename Value>
void broker<Key, Value>::unsubscribe(const Key &key, const broker::consumer_type &consumer)
{
    try {
        _partition_manager.unsubscribe(key, consumer);
    }
    catch (const broker_error &) {
        std::throw_with_nested(broker_error("Failed to unsubscribe"));
    }
}

template<typename Key, typename Value>
void broker<Key, Value>::thread_proc()
{
    while (true) {
        try {
            auto optional_value_data = _partition_manager.pop();

            if (_terminating) {
                break;
            }

            const auto &value_data = optional_value_data.value();

            for (const auto consumer: value_data.consumers) {
                consumer->consume(value_data.key, value_data.value);
            }
        }
        catch(const broker_error &e) {
            logger::debug() << e;
        }
    }
}

}