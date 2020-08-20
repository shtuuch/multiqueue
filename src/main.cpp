#include "broker.h"

#include <iostream>

class StringConsumer : public Solution::IConsumer<int, std::string>
{
public:
    void consume(const int &key, const std::string &value) const override
    {
        std::cout << "key: " << key << " " << " value:" << value << std::endl;
    }

};

int main()
{
    try {
        Solution::Broker<int, std::string> broker(10);
        auto consumer = std::make_shared<StringConsumer>();
        broker.subscribe(10, consumer);

        broker.push(10, "value1");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        consumer.reset();
        broker.push(10, "value2");
        broker.push(10, "value3");

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    catch (const Solution::BrokerError &e) {
        Solution::Logger::debug() << e;
    }

//    using Tb = Solution::IConsumer<int, std::string>;
//    using T = StringConsumer;
//    using Tw = std::weak_ptr<Tb>;
//    using Ts = std::shared_ptr<Tb>;
//
//    std::set<Tw, std::owner_less<Tw>> a;
//    a.insert(Tw(Ts(new T)));


    return 0;
}
