#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "broker.h"

namespace
{
using ::testing::Return;
using ::testing::InSequence;

template <typename Key, typename Value>
struct mockConsumer : public Solution::IConsumer<int, std::string>
{
    MOCK_CONST_METHOD2_T(consume, void(const Key &key, const Value &value));
};

}

// Tests are very basic.
// Multithreaded tests are not implemented in the scope of the assessment task.

TEST(brokerTest, consume)
{
    mockConsumer<int, std::string> stringConsumer;

    InSequence s;
    EXPECT_CALL(stringConsumer, consume(10, "value1")).WillOnce(Return());
    EXPECT_CALL(stringConsumer, consume(11, "value2")).WillOnce(Return());
    EXPECT_CALL(stringConsumer, consume(10, "value3")).WillOnce(Return());

    Solution::Broker<int, std::string> broker(10);

    broker.push(100, "unsubscribed");
    broker.push(10, "value1");
    broker.push(11, "value2");
    broker.push(10, "value3");

    broker.subscribe(10, stringConsumer);
    broker.subscribe(11, stringConsumer);

    //For simplicity sake we will not synchronize with the events count, but just wait
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(broker_test, unsubscribe)
{
    mockConsumer<int, std::string> stringConsumer;

    InSequence s;
    EXPECT_CALL(stringConsumer, consume(10, "value1")).WillOnce(Return());
    EXPECT_CALL(stringConsumer, consume(10, "value2")).WillOnce(Return());

    Solution::Broker<int, std::string> broker(10);

    broker.subscribe(10, stringConsumer);
    broker.push(10, "value1");
    broker.push(10, "value2");

    //For simplicity sake we will not synchronize with the events count, but just wait
    std::this_thread::sleep_for(std::chrono::seconds(1));

    broker.unsubscribe(10, stringConsumer);
    broker.push(10, "value3");

    //For simplicity sake we will not synchronize with the events count, but just wait
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

