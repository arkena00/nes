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
        thread_pool_executor(unsigned int n = 1) : processing_{ true }
        {
            threads_.reserve(n);

            auto thread_loop = [this]()
            {
                while (true)
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    condition_.wait(lock, [this]{ return !tasks_.empty() || !processing_; });

                    if (!processing_ && tasks_.empty()) return;

                    std::unique_lock<std::mutex> queue_lock(queue_mutex_);
                    auto pool_task = std::move(tasks_.front());
                    tasks_.pop();
                    queue_lock.unlock();
                    lock.unlock();

                    pool_task();
                }
            };

            for (int i = 0; i != n; ++i)
            {
                threads_.emplace_back(thread_loop);
            }
        }


        ~thread_pool_executor()
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                processing_ = false;
            }

            condition_.notify_all();

            for (auto& t : threads_)
            {
                t.join();
            }
        }


        template<class F, class... Args>
        auto post_fut(F f, Args&&... args)
        {
            using return_type = typename function_trait<F>::return_type;

            std::function<return_type(Args...)> function = f;

            auto shared_task = std::make_shared<nes::task<std::function<return_type(Args...)>, decltype(*this)>>(  function, *this );
            auto future = shared_task->get_future();

            auto pool_task = [shared_task, &args...]()
            {
                (*shared_task)(std::forward<Args>(args)...);
            };

            std::scoped_lock<std::mutex> lock(mutex_);
            tasks_.emplace(std::move(pool_task));
            condition_.notify_one();

            return future;
        }

        template<class Callback, class F, class... Args>
        void post(Callback&& callback, F f, Args... args)
        {
            using return_type = typename function_trait<F>::return_type;

            std::function<return_type(Args...)> function = f;

            auto shared_task = std::make_shared<nes::task<std::function<return_type(Args...)>, decltype(*this)>>(  function, *this );

            auto pool_task = [this, shared_task, &callback, args = std::make_tuple(std::forward<Args>(args)...)]()
            {
                std::apply([this, shared_task, &callback](auto&&... args){
                    auto return_value = (*shared_task)(std::move(args)...);
                    callback(std::move(return_value));
                }, std::move(args));
            };

            std::scoped_lock<std::mutex> lock(mutex_);
            tasks_.emplace(std::move(pool_task));
            condition_.notify_one();
        }




        auto& context() { return *this; }

    private:
        std::once_flag flag;

        std::vector<std::thread> threads_;
        std::queue<std::function<void()>> tasks_;
        std::mutex queue_mutex_;
        std::mutex mutex_;
        std::condition_variable condition_;
        std::atomic<bool> processing_;
    };
} // nes

#endif // THREAD_POOL_HPP_NES
