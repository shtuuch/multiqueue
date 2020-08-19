# Multiqueue broker

## Description

Class implements multiqueue broker which allows following operations:
- push Value to the queue with specified Key
- subscribe on Values corresponding to the Key
- unsubscribe from further callbacks invocations

Broker invokes subscribers' callbacks asynchroniously. Round robin algorithm is used to select next Value if there are multiple subscribers on different Keys

## Limitations

Callee cannot make a call to unsubscribe inside callback function.

## Futher enhancement

- Provide different Key selection strategies (beside round robin)
- Though unsubscribe is not supposed to be called often, still it may use not such a broad locking stategy and block only those consumers whose callback is currently being called.
- active_partitions list may use intusive containers from Boost. This would allow not to allocate memory on heap just to store pointers.

## Build
```
git clone https://github.com/shtuuch/multiqueue.git
cd multiqueue
git submodule update --init --recursive
mkdir build
cd build
cmake ..
cmake --build .
```
