#ifndef THREAD_POOL_HPP_NES
#define THREAD_POOL_HPP_NES

#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <future>
#include "task.hpp"

namespace nes
{
    class thread_pool_executor
    {
    public:
        thread_pool_executor(int n) : processing_{ true }
        {
            threads_.reserve(n);

            auto thread_loop = [this]()
            {
                while (true)
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    condition_.wait(lock, [this]{ return !tasks_.empty() || !processing_; });

                    if (!processing_ && tasks_.empty()) return;

                    auto task = std::move(tasks_.front());
                    tasks_.pop();
                    lock.unlock();

                    task();
                }
            };

            for (int i = 0; i != n; ++i)
            {
                threads_.emplace_back(thread_loop);
            }
        }
/*
        auto execute(nes::task task)
        {
            auto& exectuor = task.context();
            if (exectuor == *this) task();
            else exectuor.post(task);
        }*/

        ~thread_pool_executor()
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                processing_ = false;
            }

            condition_.notify_all();

            for (auto& t : threads_)
            {
                t.join();
            }
        }

        /*auto post(nes::task task)
        {

        }*/

        template<class F, class... Args>
        auto post(F f, Args&&... args)
        {
            using return_type = typename function_trait<F>::return_type;

            std::function<return_type(Args...)> function = f;

            nes::task task{ function, *this };
            auto future = task.get_future();

            auto shared_task = std::make_shared<decltype(task)>(std::move(task));

            auto lambda = [shared_task, &args...]()
            {
                (*shared_task)(std::forward<Args>(args)...);
            };

            std::scoped_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace(std::move(lambda));
            condition_.notify_one();

            return future;
        }

        auto& context() { return *this; }

    private:
        std::vector<std::thread> threads_;
        std::queue<std::function<void()>> tasks_;
        std::mutex queue_mutex_;
        std::condition_variable condition_;
        std::atomic<bool> processing_;
    };
} // nes

#endif // THREAD_POOL_HPP_NES
