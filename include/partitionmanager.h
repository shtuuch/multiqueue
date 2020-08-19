#pragma once

#include "exception.h"
#include "consumer.h"
#include "logger.h"

#include <condition_variable>
#include <functional>
#include <set>
#include <queue>
#include <mutex>

namespace Solution
{

template <typename Key, typename Value>
class PartitionManager
{
    using ConsumerType = IConsumer<Key, Value>;
    using ConsumerList = std::set<const ConsumerType *>;

    struct Partition
    {
        Key key;
        std::queue<Value> queue;
        ConsumerList consumers;
        bool isActive = false;
    };

    using ActivePartitionList = std::list<Partition *>;
    using ActivePartitionIterator = typename ActivePartitionList::iterator;

public:

    struct ValueContext
    {
        ValueContext(const Key &key, Value &&value, const ConsumerList &consumers, std::mutex &executionMutex)
            : key(key)
            , value(value)
            , consumers(consumers)
            , executionLock(executionMutex)
        {}

        ValueContext(const ValueContext &) = delete;

        ValueContext(ValueContext &&) noexcept = default;

        const Key &key;
        Value value;
        ConsumerList consumers;
        std::unique_lock<std::mutex> executionLock;
    };

    PartitionManager(size_t totalMaxSize);

	template <typename ...Args>
	void push(const Key &key, Args && ...args);

    std::optional<ValueContext> pop();

    void cancelWait();

	void subscribe(const Key &key, const ConsumerType &consumer);

	void unsubscribe(const Key &key, const ConsumerType &consumer);

private:

	void addConsumer(Partition &partition, const ConsumerType &consumer);

	void removeConsumer(Partition &partition, const ConsumerType &consumer);

	void removeFromActive(Partition &partition);

	void removeFromActive(ActivePartitionIterator it);

	void addToActive(Partition &partition);

	std::map<Key, Partition> _partitions;
    ActivePartitionList _activePartitions;
	ActivePartitionIterator _currentPartition;

    std::mutex _mutex;
    std::mutex _executionMutex;
    std::condition_variable _cv;

    const size_t _totalMaxSize;
    size_t _totalSize;
    bool _isWaitCanceled;
};

namespace
{
template<typename Key, typename Value>
using optionalValueContext = std::optional<typename PartitionManager<Key, Value>::ValueContext>;
}

template<typename Key, typename Value>
PartitionManager<Key, Value>::PartitionManager(size_t totalMaxSize)
    : _activePartitions()
    , _currentPartition(_activePartitions.end())
    , _totalMaxSize(totalMaxSize)
    , _totalSize(0)
    , _isWaitCanceled(false)
{}

template<typename Key, typename Value>
template<typename... Args>
void PartitionManager<Key, Value>::push(const Key &key, Args &&... args)
{
    try {
        std::unique_lock lock(_mutex);

        auto &partition = _partitions[key];
        partition.key = key;

        auto &queue = partition.queue;
        auto &consumers = partition.consumers;

        if (_totalSize == _totalMaxSize) {
            //another alternatives: either discard value or wait for items
            throw PartitionError("Maximum total size reached");
        }

        queue.emplace(std::forward<Args>(args)...);
        ++_totalSize;

        if (!partition.isActive && !consumers.empty()) {
            addToActive(partition);

            lock.unlock();
            _cv.notify_one();
        }
    }
    catch(const std::exception &) {
        std::throw_with_nested(PartitionError("Failed to push new value"));
    }
}

template<typename Key, typename Value>
optionalValueContext<Key, Value> PartitionManager<Key, Value>::pop()
{
    try {
        std::unique_lock lock(_mutex);
        _cv.wait(lock, [this] { return !_activePartitions.empty() || _isWaitCanceled; });

        if (_isWaitCanceled) {
            _isWaitCanceled = false;
            return std::nullopt;
        }

        if (_currentPartition == _activePartitions.end()) {
            _currentPartition = _activePartitions.begin();
        }

        auto &partition = *_currentPartition;
        auto &queue = partition->queue;
        std::optional<ValueContext> valueContext(std::in_place,
                                                 partition->key,
                                                 std::move(queue.front()),
                                                 partition->consumers,
                                                 _executionMutex);
        queue.pop();
        --_totalSize;

        if (queue.empty()) {
            removeFromActive(_currentPartition++);
        } else {
            ++_currentPartition;
        }

        return valueContext;
    }
    catch(const std::exception &) {
        std::throw_with_nested(PartitionError("Failed to pop value"));
    }
}

template<typename Key, typename Value>
void PartitionManager<Key, Value>::cancelWait()
{
    {
        std::lock_guard lock(_mutex);
        _isWaitCanceled = true;
    }
    _cv.notify_one();
}

template<typename Key, typename Value>
void PartitionManager<Key, Value>::subscribe(const Key &key,
                                             const PartitionManager::ConsumerType &consumer)
{
    try {
        std::unique_lock lock(_mutex);

        auto &partition = _partitions[key];
        addConsumer(partition, consumer);

        auto &queue = partition.queue;
        if (!partition.isActive && !queue.empty()) {
            addToActive(partition);

            lock.unlock();
            _cv.notify_one();
        }
    }
    catch (const PartitionError &) {
        std::throw_with_nested(PartitionError("Failed to subscribe"));
    }
}

template<typename Key, typename Value>
void PartitionManager<Key, Value>::unsubscribe(const Key &key,
                                               const PartitionManager::ConsumerType &consumer)
{
    try {
        std::scoped_lock lock(_mutex, _executionMutex);

        auto it = _partitions.find(key);
        if (it == _partitions.end()) {
            throw PartitionError("Partition for the key not found");
        }

        auto &partition = it->second;
        auto &queue = partition.queue;

        removeConsumer(partition, consumer);
        if (partition.isActive) {
            removeFromActive(partition);
        }

        if (queue.empty()) {
            _partitions.erase(it);
        }
    }
    catch (const PartitionError &) {
        std::throw_with_nested(PartitionError("Failed to unsubscribe"));
    }
}

template<typename Key, typename Value>
void PartitionManager<Key, Value>::addConsumer(PartitionManager::Partition &partition,
                                               const PartitionManager::ConsumerType &consumer)
{
    auto &consumers = partition.consumers;
    if (!consumers.insert(&consumer).second) {
        throw PartitionError("Consumer already registered for the key");
    }
}

template<typename Key, typename Value>
void PartitionManager<Key, Value>::removeConsumer(PartitionManager::Partition &partition,
                                                  const PartitionManager::ConsumerType &consumer)
{
    auto &consumers = partition.consumers;
    auto it = consumers.find(&consumer);
    if (it == consumers.end()) {
        throw PartitionError("Consumer not found");
    }

    consumers.erase(it);
}

template<typename Key, typename Value>
void PartitionManager<Key, Value>::removeFromActive(PartitionManager::Partition &partition)
{
    auto it = std::find(_activePartitions.begin(), _activePartitions.end(), &partition);
    removeFromActive(it);
}

template<typename Key, typename Value>
void PartitionManager<Key, Value>::removeFromActive(PartitionManager::ActivePartitionIterator it)
{
    if (it == _activePartitions.end()) {
        throw PartitionError("Internal error. Partition not found.");
    }

    (*it)->isActive = false;

    if (_currentPartition == it) {
        _currentPartition = _activePartitions.erase(it);
    } else {
        _activePartitions.erase(it);
    }
}

template<typename Key, typename Value>
void PartitionManager<Key, Value>::addToActive(PartitionManager::Partition &partition)
{
    _activePartitions.push_back(&partition);
    partition.isActive = true;
}

}