
#include <iostream>
#include "threadpoolc++.hpp"

void func(uint8_t *data, uint32_t len){
    for(uint32_t i = 0; i < len; i++){
        printf("%d ", data[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]){
    // create a thread pool with 3 threads
    ThreadPool::ThreadPool l_ThreadPool(3);

    while(1){
        // add a task： func with 10 bytes payload
        uint8_t l_payload[10] = {1,2,3,4,5,6,7,8,9,10};
        l_ThreadPool.addTask(func, (uint8_t *)l_payload, 10);

        // add a task： lambda with 10 bytes payload
        l_ThreadPool.addTask([]{printf("this is a lambda\n");});

        // add a task： async return value
        auto async_ret = l_ThreadPool.addTask([](uint8_t a, uint8_t b)->int{return a + b;},1,2);
        std::thread([&async_ret]{
            printf("async return: %d\n", async_ret.get());
        }).detach();

        // sleep 1ms
        std::this_thread::sleep_for(std::chrono::microseconds(1000*100));
    }
    return 0;
}