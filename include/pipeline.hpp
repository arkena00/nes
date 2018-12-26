#ifndef PIPELINE_HPP_NES
#define PIPELINE_HPP_NES

#include <array>
#include <tuple>
#include <utility.hpp>
#include <functional>

namespace nes
{
    struct inline_executor
    {
        template<class Callback, class Function, class... Args>
        void post(Callback&& callback, Function&& function, Args&&... args)
        {
            auto return_value = function(std::forward<Args>(args)...);
            callback(return_value);
        }

        static inline_executor instance;
    };

    inline_executor inline_executor::instance{};



    template<unsigned int N, unsigned int Last_index, class = void>
    struct push_result;

    //! msvc evaluate false if constexpr condition
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
            pipeline.processing_tasks_--;
            pipeline.output_condition_.notify_one();
        }
    };


    template<class F, class Executor = nes::inline_executor>
    struct pipeline_step
    {
        using input_type = typename function_trait<F>::template arg_type<0>;
        using return_type = typename function_trait<F>::return_type;

        pipeline_step(F f)
        : function{ std::move(f) }
        , executor{ nes::inline_executor::instance }
        {}

        pipeline_step(F f, Executor& e)
        : function{ std::move(f) }
        , executor{ e }
        {}

        F function;
        Executor& executor;
    };

    template<class Function>
    struct make_step
    {
        using type = pipeline_step<Function, nes::inline_executor>;
    };

    template<class Function, class Executor>
    struct make_step<pipeline_step<Function, Executor>>
    {
        using type = pipeline_step<Function, Executor>;

    };

    template<class... Steps>
    struct pipeline_trait
    {
        static constexpr auto steps_size = sizeof...(Steps);//std::tuple_size_v<Steps>;
        static constexpr auto step_last_index = steps_size - 1;

        using steps_type = std::tuple<typename make_step<Steps>::type...>;

        template <unsigned int N>
        using step_at = typename std::decay_t<std::tuple_element_t<N, steps_type>>;
        using input_type = typename step_at<0>::input_type;
        using output_type = typename step_at<step_last_index>::return_type;
    };

    template<class Input_type, class Output_type>
    class basic_pipeline
    {
    public:
        virtual void push(Input_type) = 0;
        virtual std::vector<Output_type> output() = 0;
    };

    template<class... Steps>
    class pipeline : public basic_pipeline<typename pipeline_trait<Steps...>::input_type, typename pipeline_trait<Steps...>::output_type>
    {
    public:
        template<unsigned int N, unsigned int Last_index, class>
        friend struct push_result;

        static constexpr auto steps_size = sizeof...(Steps);//std::tuple_size_v<Steps>;
        static constexpr auto step_last_index = steps_size - 1;

        using steps_type = typename pipeline_trait<Steps...>::steps_type;
        using input_type = typename pipeline_trait<Steps...>::input_type;
        using output_type = typename pipeline_trait<Steps...>::output_type;

        pipeline(Steps... steps)
            : processing_{ true }
            , processing_tasks_{ 0 }
            , steps_{ std::move(steps)... }
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
                        step_condition<N>().wait(lock, [this, index]{ auto N = decltype(index){}; return !std::get<N>(step_input_).empty() || !processing_; });

                        if (!processing_) return;

                        // get data to process
                        auto process_data = std::move(std::get<N>(step_input_).front());
                        std::get<N>(step_input_).pop();
                        lock.unlock();

                        auto& pipeline_task = step<N>().function;
                        auto& executor = step<N>().executor;


                        executor.post(
                        [this, index](auto&& return_value){ auto N = decltype(index){}; push_result<N, step_last_index>::process(*this, std::move(return_value)); }
                        , pipeline_task
                        , process_data);

                        // nes::exec(pipeline_task, step<N>().executor, [](auto&& return_value){ ... });

                        //auto return_value = pipeline_task(process_data);

                        // push result
                        //push_result<N, step_last_index>::process(*this, std::move(return_value));
                    }
                };

                std::get<N>(step_thread_) = std::thread{thread_loop};
            });
        }

        pipeline(pipeline&&) noexcept = default;

        ~pipeline()
        {
            processing_ = false;

            nes::for_each(steps_, [this](auto&& index)
            {
                auto N = decltype(index){};

                step_condition<N>().notify_one();
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
            std::get<N>(step_input_).push(std::move(value));
            lock.unlock();

            step_condition<N>().notify_one();
        }

        void push(input_type v) override
        {
            //inputs_.push(v);
            processing_tasks_++;
            step_push<0>(std::move(v));
        }

        /*
        void cancel(input_value)
        {

        }*/

        void wait()
        {
            std::unique_lock<std::mutex> lock(output_mutex_);
            output_condition_.wait(lock, [this]{ return processing_tasks_ == 0; });
        }

        // drain the current output
        std::vector<output_type> output() override
        {
            std::vector<output_type> return_output;

            std::unique_lock<std::mutex> lock_output(output_mutex_);
            return_output.swap(output_);
            lock_output.unlock();

            return return_output;
        }

    private:
        std::atomic<bool> processing_;
        std::atomic<unsigned int> processing_tasks_;
        steps_type steps_;

        //std::priority_queue<input_type, std::vector<input_type>, std::greater<input_type>> inputs_;

        std::tuple<std::queue<typename pipeline_trait<Steps>::input_type>...> step_input_;
        std::array<std::thread, steps_size> step_thread_;
        std::array<std::mutex, steps_size> step_mutex_;
        std::array<std::condition_variable, steps_size> step_condition_;

        std::mutex output_mutex_;
        std::condition_variable output_condition_;
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
