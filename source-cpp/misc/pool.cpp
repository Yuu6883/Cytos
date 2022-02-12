#include "pool.hpp"
#include "logger.hpp"

#ifdef WIN32
#include <Windows.h>
#else 
// TODO: implement pthread cpu binding
#include <pthread>
#endif

ThreadPool::ThreadPool(uint32_t n) : busy(0), processed(0), stop(0) {
    if (n <= 0) {
        n = 1;
        logger::warn("Setting thread pool worker to 1\n");
    }

    auto total_cpus = std::thread::hardware_concurrency();
    auto ratio = total_cpus / n;

    // TODO: learn how to use hwloc instead of this crap
    
    // step = 1: cpu 0,1,2,3...
    // step = 2: cpu 0,2,4,6... (good for hyperthreaded processor?)
    // step = 4: cpu 0,4,8,12... 
    // etc

    auto step = 1;
    while ((step << 1) <= ratio) step = step << 1;

    for (uint32_t i = 0; i < n; ++i) {
        workers.emplace_back(std::bind(&ThreadPool::thread_proc, this, i * step));
    }
}

ThreadPool::~ThreadPool() {
    // set stop-condition
    std::unique_lock<std::mutex> latch(queue_mutex);
    stop = true;
    cv_task.notify_all();
    latch.unlock();

    // all threads terminate, then we're done.
    for (auto& t : workers)
        t.join();
}

void ThreadPool::thread_proc(uint32_t cpu) {
#ifdef WIN32
    SetThreadAffinityMask(GetCurrentThread(), (1 << cpu));
#else
// TODO
#endif

    while (true) {
        std::unique_lock<std::mutex> latch(queue_mutex);
        cv_task.wait(latch, [this]() { return stop || !tasks.empty(); });
        if (!tasks.empty()) {
            // got work. set busy.
            ++busy;

            // pull from queue
            auto fn = tasks.front();
            tasks.pop_front();

            // release lock. run async
            latch.unlock();

            // run function outside context
            fn();
            ++processed;

            latch.lock();
            --busy;
            cv_finished.notify_one();
        } else if (stop) break;
    }
}

void ThreadPool::enqueue(std::function<void(void)> f) {
    if (workers.size()) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace_back(std::forward<std::function<void(void)>>(f));
        cv_task.notify_one();
    } else f();
}

// waits until the queue is empty.
void ThreadPool::sync() {
    if (workers.size()) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv_finished.wait(lock, [this]() { return tasks.empty() && (busy == 0); });
    }
}