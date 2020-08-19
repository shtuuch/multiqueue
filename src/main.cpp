#include "broker.h"

#include <iostream>

class string_consumer : public solution::consumer<int, std::string>
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
        solution::broker<int, std::string> broker(10);
        string_consumer consumer;
        broker.subscribe(10, consumer);

        broker.push(10, "value1");
        broker.push(10, "value2");
        broker.push(10, "value3");

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    catch (const solution::broker_error &e) {
        solution::logger::debug() << e;
    }

    return 0;
}
