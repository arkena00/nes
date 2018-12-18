#ifndef SERIAL_EXECUTOR_HPP_NES
#define SERIAL_EXECUTOR_HPP_NES

#include <tuple>
#include <thread>
#include <queue>
#include "utility.hpp"

namespace nse
{
    template<class Tasks>
    class serial_executor
    {
        static constexpr auto task_chain_size = std::tuple_size_v<Tasks>;

    public:
        constexpr serial_executor(Tasks tasks) : tasks_{ std::move(tasks) }
        {
            nes::for_each(tasks_, [this](auto&& index)
            {
                auto N = decltype(index){};

                auto thread_loop = [this, N]()
                {
                    auto future = std::get<0>(tasks_).get_future();
                };

                std::get<N>(threads_) = std::thread{ thread_loop };
            });
        }

        void push(int a)
        {
            datas_[0].emplace(a);
            threads_[0].notify_one();
        }

    private:
        Tasks tasks_;
        std::array<std::thread, task_chain_size> threads_;
        std::array<std::queue<int>, task_chain_size> datas_;
    };
} // nse

#endif // SERIAL_EXECUTOR_HPP_NES
