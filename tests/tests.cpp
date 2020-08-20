#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "broker.h"

namespace
{
using ::testing::Return;
using ::testing::InSequence;

constexpr size_t maxItems = 10;

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
    auto stringConsumer = std::make_shared<mockConsumer<int, std::string>>();

    EXPECT_CALL(*stringConsumer, consume(1, "value1")).WillOnce(Return());
    EXPECT_CALL(*stringConsumer, consume(2, "value2")).WillOnce(Return());
    EXPECT_CALL(*stringConsumer, consume(1, "value3")).WillOnce(Return());

    Solution::Broker<int, std::string> broker(maxItems);

    broker.push(3, "not_assigned");
    broker.push(1, "value1");
    broker.push(2, "value2");
    broker.push(1, "value3");


    broker.subscribe(1, stringConsumer);
    broker.subscribe(2, stringConsumer);

    //For simplicity sake we will not synchronize with the events count, but just wait
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(broker_test, unsubscribe)
{
    auto stringConsumer = std::make_shared<mockConsumer<int, std::string>>();

    InSequence s;
    EXPECT_CALL(*stringConsumer, consume(1, "value1")).WillOnce(Return());
    EXPECT_CALL(*stringConsumer, consume(1, "value2")).WillOnce(Return());

    Solution::Broker<int, std::string> broker(maxItems);

    broker.subscribe(1, stringConsumer);
    broker.push(1, "value1");
    broker.push(1, "value2");

    //For simplicity sake we will not synchronize with the events count, but just wait
    std::this_thread::sleep_for(std::chrono::seconds(1));

    broker.unsubscribe(1, stringConsumer);
    broker.push(1, "value3");

    //For simplicity sake we will not synchronize with the events count, but just wait
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(broker_test, unsubscribe_inside_callback)
{
    auto stringConsumer1 = std::make_shared<mockConsumer<int, std::string>>();
    auto stringConsumer2 = std::make_shared<mockConsumer<int, std::string>>();
    Solution::Broker<int, std::string> broker(maxItems);

    InSequence s;
    EXPECT_CALL(*stringConsumer1, consume(1, "value1")).WillOnce([&](int key, const std::string &) {
        broker.unsubscribe(1, stringConsumer1);
        broker.unsubscribe(2, stringConsumer2);
    });

    broker.subscribe(1, stringConsumer1);
    broker.subscribe(2, stringConsumer2);
    broker.push(1, "value1");
    broker.push(1, "value2");
    broker.push(2, "value1");
    broker.push(2, "value2");

    //For simplicity sake we will not synchronize with the events count, but just wait
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(broker_test, consumer_reset_inside_callback)
{
    auto stringConsumer1 = std::make_shared<mockConsumer<int, std::string>>();
    auto stringConsumer2 = std::make_shared<mockConsumer<int, std::string>>();
    Solution::Broker<int, std::string> broker(maxItems);

    InSequence s;
    EXPECT_CALL(*stringConsumer1, consume(1, "value1")).WillOnce([&](int key, const std::string &) {
        stringConsumer1.reset();
        stringConsumer2.reset();
    });

    broker.subscribe(1, stringConsumer1);
    broker.subscribe(2, stringConsumer2);
    broker.push(1, "value1");
    broker.push(1, "value2");
    broker.push(2, "value1");
    broker.push(2, "value2");

    //For simplicity sake we will not synchronize with the events count, but just wait
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

