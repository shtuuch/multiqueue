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
        StringConsumer consumer;
        broker.subscribe(10, consumer);

        broker.push(10, "value1");
        broker.push(10, "value2");
        broker.push(10, "value3");

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    catch (const Solution::BrokerError &e) {
        Solution::Logger::debug() << e;
    }

    return 0;
}
