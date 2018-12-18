#ifndef PIPELINE_HPP_NES
#define PIPELINE_HPP_NES

#include <array>
#include <tuple>
#include <utility.hpp>
#include <functional>

namespace nes
{
    template<unsigned int N, unsigned int Last_index, class = void>
    struct push_result;

    // has next step
    template<unsigned int N, unsigned int Last_index>
    struct push_result<N, Last_index, std::enable_if_t<N < Last_index>>
    {
        template<class Pipeline, class Value>
        static void process(Pipeline& pipeline, Value&& value)
        {
            pipeline.template step_push<N + 1>(std::move(value));
        }
    };

    // last_step
    template<unsigned int N, unsigned int Last_index>
    struct push_result<N, Last_index, std::enable_if_t<N == Last_index>>
    {
        template<class Pipeline, class Value>
        static void process(Pipeline& pipeline, Value&& value)
        {
            std::unique_lock<std::mutex> lock_output(pipeline.output_mutex_);
            pipeline.output_.emplace_back(std::move(value));
            lock_output.unlock();
        }
    };


    template<class F>
    struct pipeline_step
    {
        using input_type = typename function_trait<F>::template arg_type<0>;
        using return_type = typename function_trait<F>::return_type;

        pipeline_step(F f) : function{ std::move(f) } {}

        F function;
    };


    template<class... Steps>
    class pipeline
    {
    public:
        template<unsigned int N, unsigned int Last_index, class>
        friend struct push_result;

        static constexpr auto steps_size = sizeof...(Steps);//std::tuple_size_v<Steps>;
        static constexpr auto step_last_index = steps_size - 1;

        template <unsigned int N>
        using step_at = typename std::decay_t<std::tuple_element_t<N, std::tuple<Steps...>>>;
        using input_type = typename step_at<0>::input_type;
        using output_type = typename step_at<step_last_index>::return_type;

        pipeline(Steps&&... steps)
            : processing_{ true }
            , steps_{ std::forward_as_tuple(steps...) }
        {
            nes::for_each(steps_, [this](auto&& index)
            {
                auto N = decltype(index){};

                // thread loop
                auto thread_loop = [this, index]()
                {
                    while (true)
                    {
                        auto N = decltype(index){};

                        std::unique_lock<std::mutex> lock(std::get<N>(step_mutex_));
                        step_condition<N>().wait(lock, [this, index]{ auto N = decltype(index){}; return !std::get<N>(step_data_).empty() || !processing_; });

                        if (!processing_) return;

                        // get data to process
                        auto s = std::get<N>(step_data_).size();
                        auto process_data = std::get<N>(step_data_).front();
                        std::get<N>(step_data_).pop();
                        lock.unlock();

                        auto& pipeline_task = step<N>().function;

                        auto return_value = pipeline_task(0);
                        //std::cout << "V : " << return_value;

                        // push result
                        push_result<N, step_last_index>::process(*this, std::move(return_value));
                    }
                };

                std::get<N>(step_thread_) = std::thread{thread_loop};

            });
        }


        ~pipeline()
        {
            nes::for_each(steps_, [this](auto&& index)
            {
                auto N = decltype(index){};

                std::get<N>(step_thread_).join();
            });
        }

        template<unsigned int N>
        auto& step_condition() { return std::get<N>(step_condition_); }


        template<unsigned int N>
        auto& step() { return std::get<N>(steps_); }

        template<unsigned int N, class T>
        void step_push(T value)
        {
            std::unique_lock<std::mutex> lock(std::get<N>(step_mutex_));
            std::get<N>(step_data_).push(std::move(value));
            auto s = std::get<N>(step_data_).size();
            lock.unlock();

            step_condition<N>().notify_one();
        }

        template<class T>
        void push(T v)
        {
            std::cout << "push0";
            static_assert(std::is_same_v<T, input_type>, "Invalid input type");

            //inputs_.push(v);
            step_push<0>(std::move(v));
        }

        /*
        void cancel(input_value)
        {

        }*/

        std::vector<output_type> output()
        {
            std::vector<output_type> return_output;

            std::unique_lock<std::mutex> lock_output(output_mutex_);
            return_output.swap(output_);
            lock_output.unlock();

            return return_output;
        }

    private:
        bool processing_;
        std::tuple<Steps...> steps_;


        std::tuple<std::queue<typename Steps::input_type>...> step_data_;


        //std::priority_queue<input_type, std::vector<input_type>, std::greater<input_type>> inputs_;

        std::array<std::thread, steps_size> step_thread_;
        std::array<std::mutex, steps_size> step_mutex_;
        std::array<std::condition_variable, steps_size> step_condition_;

        std::mutex output_mutex_;
        std::vector<output_type> output_;
    };

    /*
    template<class... Functions>
    auto&& make_pipeline(Functions&&... functions)
    {
        nes::pipeline p{ std::forward_as_tuple(nes::pipeline_step{std::forward<Functions>(functions)}...) };
        return std::move(p);
    }*/

} // nes


#endif // PIPELINE_HPP_NES
