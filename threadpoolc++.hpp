#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>

namespace ThreadPool
{

#define ADMIN_TIME 100// 守护间隔时间
#define MIN_THREAD_NUM 5// 最小线程数
#define MAX_THREAD_NUM 15// 最大线程数
#define DEFAULT_THREAD_NUM 2// 单次创建/销毁线程数
#define MIN_WAIT_TASK_NUM 20// 最小等待任务数
#define MAX_QUEZE_SIZE 500// 最大任务数

class ThreadPool {
public:
    ThreadPool(size_t _threads_num):min_thread_num(_threads_num),stop(false){
        std::thread([this]{
            for(;;){
                uint16_t task_size = 0, live_thr_size = 0, busy_task_size = 0;
                {
                    std::unique_lock<std::mutex> lock(this->admin_mutex);
                    task_size = this->tasks.size();
                    live_thr_size = this->live_thr_size;
                    busy_task_size = this->busy_task_size;
                    std::cout << "task_size: " << task_size << " live_thr_size: " << live_thr_size << " busy_task_size: " << busy_task_size << std::endl;
                }
                // 创建线程条件: 实际任务数 > (最小等待的任务数 + 2倍存活线程数）&& 存活线程数 < 最大线程数
                if (task_size >= (MIN_WAIT_TASK_NUM + live_thr_size * 2) && live_thr_size <= max_thread_num){
                    newThread(DEFAULT_THREAD_NUM);
                }
                // 销毁线程条件：工作线程 < 2 && 任务队列 < 2 &&存活线程数大于最小线程数*/
                if (busy_task_size < 2  && task_size < 2 && live_thr_size > min_thread_num){
                    delThread(DEFAULT_THREAD_NUM);
                }            
                std::this_thread::sleep_for(std::chrono::milliseconds(admin_time));
            }
        }).detach();
        newThread(min_thread_num);
    }

    ~ThreadPool(){
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
        condition.notify_all();
    }

    template<class F, class... Args>
    auto addTask(F&& _f, Args&&... _args) -> std::future<typename std::result_of<F(Args...)>::type>{
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared< std::packaged_task<return_type()> >(
                std::bind(std::forward<F>(_f), std::forward<Args>(_args)...)
            );
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if(tasks.size() < max_queze_size && !stop)
                tasks.emplace([task](){ (*task)(); });
        }
        condition.notify_one();
        return res;  
    }

    void setThreadConfig(size_t _admin_time = ADMIN_TIME, size_t _max_thread_num = MAX_THREAD_NUM, size_t _max_queze_size = MAX_QUEZE_SIZE){
        admin_time = _admin_time;
        max_thread_num = _max_thread_num;
        max_queze_size = _max_queze_size;
    }
private:
    void newThread(size_t _threads_num = DEFAULT_THREAD_NUM){
        std::unique_lock<std::mutex> lock(thread_mutex);
        for(size_t i=0; (i<_threads_num && live_thr_size<=max_thread_num); ++i){
            std::thread([this]{
                this->live_thr_size++;
                std::cout<<"new thread: "<<std::this_thread::get_id()<<std::endl;
                for(;;){
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    {
                        std::unique_lock<std::mutex> lock(this->admin_mutex);
                        this->busy_task_size++;
                    }            
                    task();
                    {
                        std::unique_lock<std::mutex> lock(this->admin_mutex);
                        this->busy_task_size--;
                        if(this->wait_exit_size > 0 && this->live_thr_size > this->min_thread_num){
                            std::cout<<"del thread: "<<std::this_thread::get_id()<<std::endl;
                            this->wait_exit_size--;
                            this->live_thr_size--;
                            break;
                        }
                    }
                }
            }).detach();
        }  
    }

    void delThread(size_t _threads_num = DEFAULT_THREAD_NUM){this->wait_exit_size = _threads_num;}
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::mutex admin_mutex;
    std::mutex thread_mutex;
    std::condition_variable condition;
    size_t live_thr_size = 0;
    size_t busy_task_size = 0;
    size_t wait_exit_size = 0;
    size_t admin_time = ADMIN_TIME;
    size_t min_thread_num = MIN_THREAD_NUM;
    size_t max_thread_num = MAX_THREAD_NUM;
    size_t max_queze_size = MAX_QUEZE_SIZE;
    bool stop;
};

}