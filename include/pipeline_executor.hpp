#ifndef PIPELINE_EXECUTOR_HPP_NES
#define PIPELINE_EXECUTOR_HPP_NES

#include <array>
#include <task.hpp>
#include <utility.hpp>

namespace nes
{
    /*
    template<class T>
    struct pipeline_step
    {
        // data_id

        std::thread thread_;
        std::queue<T> datas_;

        std::mutex queue_mutex_;
        std::condition_variable conditions_;
    };*/


    template<class Tasks>
    class static_pipeline
    {
        using input_type = int;
        static constexpr auto tasks_size = std::tuple_size_v<Tasks>;//std::decay_t<Task_chain>::size();
        static constexpr auto task_last_index = tasks_size - 1;

    public:
        constexpr static_pipeline(Tasks tasks) : tasks_{ std::move(tasks) }
        {
            nes::for_each(tasks_, [this](auto&& index)
            {
                auto N = decltype(index){};

                // thread loop
                auto thread_loop = [this, index]()
                {
                    while (true)
                    {
                        auto N = decltype(index){};

                        std::unique_lock<std::mutex> lock(data_mutex_[N]);
                        conditions_[N].wait(lock, [this, index]{ return !std::get<decltype(index){}>(datas_).empty(); });


                        //if (!processing_ && std::get<N>(datas_).empty()) return;

                        // get data to process
                        auto process_data = std::move(std::get<N>(datas_).front());
                        std::get<N>(datas_).pop();
                        lock.unlock();

                        auto& pipeline_task = std::get<N>(tasks_);

                        auto return_value = pipeline_task(process_data);


                        // push result
                        if constexpr (N < task_last_index)
                        {
                            std::unique_lock<std::mutex> lock_next(data_mutex_[N + 1]);
                            datas_[N + 1].emplace(return_value);
                            lock_next.unlock();
                            conditions_[N + 1].notify_one();
                        } else
                        {
                            std::unique_lock<std::mutex> lock_output(output_mutex_);
                            output_.emplace(return_value);
                            lock_output.unlock();

                            std::cout << "\nFINAL PUSH" << return_value;

                        }
                    }
                };

                std::get<N>(threads_) = std::thread{thread_loop};
            });
        }

        ~static_pipeline()
        {
            nes::for_each(tasks_, [this](auto&& index)
            {
                auto N = decltype(index){};

                std::get<N>(threads_).join();
            });
        }

        template<class T> // T == Tasks<0>
        void push(T v)
        {
            std::cout << "\n" << "PUSH " << v;
            std::scoped_lock<std::mutex> lock(data_mutex_[0]);
            datas_[0].emplace(v);

            conditions_[0].notify_one();
        }

        /*
        void cancel(input_value)
        {

        }*/

        auto output()
        {
            std::scoped_lock<std::mutex> lock_output(output_mutex_);
            return output_;
        }

    private:
        //std::array<nes::pipeline_step<input_type>, tasks_size> steps_;

        Tasks tasks_;
        std::array<std::thread, tasks_size> threads_;
        std::array<std::queue<input_type>, tasks_size> datas_;

        std::mutex output_mutex_;
        std::queue<input_type> output_;

        std::array<std::mutex, tasks_size> data_mutex_;
        std::array<std::condition_variable, tasks_size> conditions_;
    };
} // nes


#endif // PIPELINE_EXECUTOR_HPP_NES
