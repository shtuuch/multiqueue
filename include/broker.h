#pragma once

#include "exception.h"
#include "partitionmanager.h"
#include "consumer.h"
#include "logger.h"

#include <condition_variable>
#include <functional>
#include <sstream>
#include <set>
#include <queue>
#include <mutex>
#include <atomic>

namespace Solution
{

template <typename Key, typename Value>
class Broker
{
using ConsumerWeak = std::weak_ptr<IConsumer<Key, Value>>;
using ConsumerShared = std::shared_ptr<IConsumer<Key, Value>>;

public:
    explicit Broker(size_t maxSize);

    ~Broker();

	template <typename ...Args>
	void push(const Key &key, Args && ...args);

	void subscribe(const Key &key, const ConsumerWeak &consumer);

    template<typename ConsumerType>
    void unsubscribe(const Key &key, const ConsumerType &consumer);

private:
	void threadProc();

    PartitionManager<Key, Value> _partitionManager;
    std::atomic_bool _terminating;
    std::thread _workerThread;
};

template<typename Key, typename Value>
Broker<Key, Value>::Broker(size_t maxSize)
    : _partitionManager(maxSize)
    , _terminating(false)
    , _workerThread(std::bind(&Broker::threadProc, this))
{}

template<typename Key, typename Value>
Broker<Key, Value>::~Broker()
{
    _terminating = true;
    _partitionManager.cancelWait();
    _workerThread.join();
}

template<typename Key, typename Value>
template<typename... Args>
void Broker<Key, Value>::push(const Key &key, Args &&... args)
{
    _partitionManager.push(key, std::forward<Args>(args)...);
}

template<typename Key, typename Value>
void Broker<Key, Value>::subscribe(const Key &key, const Broker::ConsumerWeak &consumer)
{
    try {
        _partitionManager.subscribe(key, consumer);
    }
    catch (const BrokerError &) {
        std::throw_with_nested(BrokerError("Failed to subscribe"));
    }
}

template<typename Key, typename Value>
template<typename ConsumerType>
void Broker<Key, Value>::unsubscribe(const Key &key, const ConsumerType &consumer)
{
    static_assert(std::is_convertible_v<ConsumerType, ConsumerWeak> ||
                 std::is_convertible_v<ConsumerType, ConsumerShared>, "Should be shared_ptr or weak_ptr");
    try {
        _partitionManager.unsubscribe(key, consumer);
    }
    catch (const BrokerError &) {
        std::throw_with_nested(BrokerError("Failed to unsubscribe"));
    }
}

template<typename Key, typename Value>
void Broker<Key, Value>::threadProc()
{
    while (true) {
        try {
            auto optionalValueContext = _partitionManager.pop();

            if (_terminating) {
                break;
            }

            const auto &valueContext = optionalValueContext.value();

            for (const auto &consumer: valueContext.consumers) {
                consumer->consume(valueContext.key, valueContext.value);
            }
        }
        catch(const BrokerError &e) {
            Logger::debug() << e;
        }
    }
}

}