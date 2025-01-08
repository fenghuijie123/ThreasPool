# ThreasPool
基于C++的线程池实现

## 功能
1. 多任务并发
2. 任务函数可变参/异步返回
3. 守护线程管理/线程数动态增减

## 介绍
### 线程池参数
#### ADMIN_TIME
线程池守护间隔时间，默认值100ms。守护线程根据任务数、存活线程数、活跃线程数动态调整线程数量，以便处理峰值数据。

#### MIN_THREAD_NUM
线程池最小线程数，指线程池运行时默认最少存活线程，默认值5，可在实例化时设置

#### MAX_THREAD_NUM
线程池最大线程数，指线程池运行时默认最多存活线程，默认值15，可在实例化时设置

#### DEFAULT_THREAD_NUM
单次创建/销毁的线程数，指线程运行时动态创建/销毁线程的个数，默认值2，可在实例化时设置

#### MIN_WAIT_TASK_NUM
最小等待任务数，线程池运行时增加线程的关键条件，当任务数大于最小等待任务数 && 2倍的存活线程数时将创建新的线程。当工作线程＜2 && 任务数<2时将删除动态创建的线程。（备注：线程增删的条件可根据实际任务调整）

#### MAX_QUEZE_SIZE
最大任务数，指线程池支持的最大任务数量，若任务数达到最大任务数时，将出现任务丢失。需要隔离配置最小等待任务数保证及时创建新的线程处理任务函数。

### 线程池调用示例
```cpp
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
```