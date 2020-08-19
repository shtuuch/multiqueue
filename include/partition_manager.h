#pragma once

#include "broker_exception.h"
#include "consumer.h"
#include "logger.h"

#include <condition_variable>
#include <functional>
#include <set>
#include <queue>
#include <mutex>

namespace solution
{

template <typename Key, typename Value>
class partition_manager
{
    using consumer_type = consumer<Key, Value>;
    using consumer_collection = std::set<const consumer_type *>;

    struct partition
    {
        Key key;
        std::queue<Value> queue;
        consumer_collection consumers;
        bool is_active = false;
    };

    using active_partition_list = std::list<partition *>;
    using active_partition_iterator = typename active_partition_list::iterator;

public:

    struct value_info
    {
        value_info(const Key &key, Value &&value, const consumer_collection &consumers, std::mutex &execution_mutex)
            : key(key)
            , value(value)
            , consumers(consumers)
            , execution_lock(execution_mutex)
        {}

        value_info(const value_info &) = delete;

        value_info(value_info &&) noexcept = default;

        const Key &key;
        Value value;
        consumer_collection consumers;
        std::unique_lock<std::mutex> execution_lock;
    };

    partition_manager(size_t total_max_size);

	template <typename ...Args>
	void push(const Key &key, Args && ...args);

    std::optional<value_info> pop();

    void cancel_wait();

	void subscribe(const Key &key, const consumer_type &consumer);

	void unsubscribe(const Key &key, const consumer_type &consumer);

private:

	void add_consumer(partition &partition, const consumer_type &consumer);

	void remove_consumer(partition &partition, const consumer_type &consumer);

	void remove_from_active(partition &partition);

	void remove_from_active(active_partition_iterator it);

	void add_to_active(partition &partition);

	std::map<Key, partition> _partitions;
    active_partition_list _active_partitions;
	active_partition_iterator _current_partition;

    std::mutex _mutex;
    std::mutex _execution_mutex;
    std::condition_variable _cv;

    const size_t _total_max_size;
    size_t _total_size;
    bool _is_wait_canceled;
};

namespace
{
template<typename Key, typename Value>
using optional_value_info = std::optional<typename partition_manager<Key, Value>::value_info>;
}

template<typename Key, typename Value>
partition_manager<Key, Value>::partition_manager(size_t total_max_size)
    : _active_partitions()
    , _current_partition(_active_partitions.end())
    , _total_max_size(total_max_size)
    , _total_size(0)
    , _is_wait_canceled(false)
{}

template<typename Key, typename Value>
template<typename... Args>
void partition_manager<Key, Value>::push(const Key &key, Args &&... args)
{
    try {
        std::unique_lock lock(_mutex);

        auto &partition = _partitions[key];
        partition.key = key;

        auto &queue = partition.queue;
        auto &consumers = partition.consumers;

        if (_total_size == _total_max_size) {
            //another alternatives: either discard value or wait for items
            throw partition_error("Maximum total size reached");
        }

        queue.emplace(std::forward<Args>(args)...);
        ++_total_size;

        if (!partition.is_active && !consumers.empty()) {
            add_to_active(partition);

            lock.unlock();
            _cv.notify_one();
        }
    }
    catch(const std::exception &) {
        std::throw_with_nested(partition_error("Failed to push new value"));
    }
}

template<typename Key, typename Value>
optional_value_info<Key, Value> partition_manager<Key, Value>::pop()
{
    try {
        std::unique_lock lock(_mutex);
        _cv.wait(lock, [this] { return !_active_partitions.empty() || _is_wait_canceled; });

        if (_is_wait_canceled) {
            _is_wait_canceled = false;
            return std::nullopt;
        }

        if (_current_partition == _active_partitions.end()) {
            _current_partition = _active_partitions.begin();
        }

        auto &partition = *_current_partition;
        auto &queue = partition->queue;
        std::optional<value_info> value_data(std::in_place,
                                             partition->key,
                                             std::move(queue.front()),
                                             partition->consumers,
                                             _execution_mutex);
        queue.pop();
        --_total_size;

        if (queue.empty()) {
            remove_from_active(_current_partition++);
        } else {
            ++_current_partition;
        }

        return value_data;
    }
    catch(const std::exception &) {
        std::throw_with_nested(partition_error("Failed to pop value"));
    }
}

template<typename Key, typename Value>
void partition_manager<Key, Value>::cancel_wait()
{
    {
        std::lock_guard lock(_mutex);
        _is_wait_canceled = true;
    }
    _cv.notify_one();
}

template<typename Key, typename Value>
void partition_manager<Key, Value>::subscribe(const Key &key,
                                              const partition_manager::consumer_type &consumer)
{
    try {
        std::unique_lock lock(_mutex);

        auto &partition = _partitions[key];
        add_consumer(partition, consumer);

        auto &queue = partition.queue;
        if (!partition.is_active && !queue.empty()) {
            add_to_active(partition);

            lock.unlock();
            _cv.notify_one();
        }
    }
    catch (const partition_error &) {
        std::throw_with_nested(partition_error("Failed to subscribe"));
    }
}

template<typename Key, typename Value>
void partition_manager<Key, Value>::unsubscribe(const Key &key,
                                                const partition_manager::consumer_type &consumer)
{
    try {
        std::scoped_lock lock(_mutex, _execution_mutex);

        auto it = _partitions.find(key);
        if (it == _partitions.end()) {
            throw partition_error("Partition for the key not found");
        }

        auto &partition = it->second;
        auto &queue = partition.queue;

        remove_consumer(partition, consumer);
        if (partition.is_active) {
            remove_from_active(partition);
        }

        if (queue.empty()) {
            _partitions.erase(it);
        }
    }
    catch (const partition_error &) {
        std::throw_with_nested(partition_error("Failed to unsubscribe"));
    }
}

template<typename Key, typename Value>
void partition_manager<Key, Value>::add_consumer(partition_manager::partition &partition,
                                                 const partition_manager::consumer_type &consumer)
{
    auto &consumers = partition.consumers;
    if (!consumers.insert(&consumer).second) {
        throw partition_error("Consumer already registered for the key");
    }
}

template<typename Key, typename Value>
void partition_manager<Key, Value>::remove_consumer(partition_manager::partition &partition,
                                                    const partition_manager::consumer_type &consumer)
{
    auto &consumers = partition.consumers;
    auto it = consumers.find(&consumer);
    if (it == consumers.end()) {
        throw partition_error("Consumer not found");
    }

    consumers.erase(it);
}

template<typename Key, typename Value>
void partition_manager<Key, Value>::remove_from_active(partition_manager::partition &partition)
{
    auto it = std::find(_active_partitions.begin(), _active_partitions.end(), &partition);
    remove_from_active(it);
}

template<typename Key, typename Value>
void partition_manager<Key, Value>::remove_from_active(partition_manager::active_partition_iterator it)
{
    if (it == _active_partitions.end()) {
        throw partition_error("Internal error. Partition not found.");
    }

    (*it)->is_active = false;

    if (_current_partition == it) {
        _current_partition = _active_partitions.erase(it);
    } else {
        _active_partitions.erase(it);
    }
}

template<typename Key, typename Value>
void partition_manager<Key, Value>::add_to_active(partition_manager::partition &partition)
{
    _active_partitions.push_back(&partition);
    partition.is_active = true;
}

}