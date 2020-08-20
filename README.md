# Multiqueue broker

## Description

Class implements multiqueue broker which allows following operations:
- push Value to the queue with specified Key
- subscribe on Values corresponding to the Key
- unsubscribe from further callbacks invocations

Broker invokes subscribers' callbacks asynchronously. Round robin algorithm is used to select next Value if there are multiple subscribers on different Keys

##Limitations

Call to unsubscribe inside callback will succeed but the all consumers for the value will be called.

## Further enhancement

- Provide different Key selection strategies (beside round robin)
- active_partitions list may use intrusive containers from Boost. This would allow not to allocate memory on heap just to store pointers.

## Build

git clone https://github.com/shtuuch/multiqueue.git
cd multiqueue
git submodule update --init --recursive
mkdir build
cd build
cmake ..
cmake --build .

