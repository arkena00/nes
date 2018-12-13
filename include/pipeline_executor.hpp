#ifndef PIPELINE_EXECUTOR_HPP_NES
#define PIPELINE_EXECUTOR_HPP_NES

#include <array>
#include <task.hpp>
#include <utility.hpp>

namespace nes
{
    template<class Task_chain>
    class static_pipeline
    {
        static constexpr auto task_chain_size = std::decay_t<Task_chain>::size();

    public:
        static_pipeline(Task_chain task_chain) : task_chain_{ task_chain }
        {
            nes::for_each(task_chain_, [this](auto&& index)
            {
                auto N = decltype(index){};

                auto thread_loop = [N]()
                {
                    while (true)
                    {
                        /*
                        std::unique_lock<std::mutex> lock(queue_mutex_);

                        condition_.wait(lock, [this]{ return N == task_chain_index_|| !processing_; });

                        task();
                        task_chain_index_++ % N;
                        condition_.notify_one();
                         */
                    }
                };

                //threads_.emplace_back(thread_loop);
            });
        }

        template<class T>
        void post(T)
        {
            //task_chain_.process(0);
        }


        //Executor executor_;
        unsigned int task_chain_index_;
        Task_chain task_chain_;
        std::array<std::thread, task_chain_size> threads_;
        std::condition_variable condition_;
    };
} // nes

#endif // PIPELINE_EXECUTOR_HPP_NES

/*
1  2  3  4  5
|2    |1
*/