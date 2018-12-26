#ifndef TASK_HPP_NES
#define TASK_HPP_NES

#include <functional>
#include <future>

namespace nes
{

    template<class T>
struct function_trait : public function_trait<decltype(&std::decay_t<T>::operator())> {};

    template <class Class, class Return_type, class... Args>
    struct function_trait<Return_type(Class::*)(Args...) const>
    {
        using return_type = Return_type;
        using function_type = std::function<return_type(Args...)>;

        template <unsigned int N>
        using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };



    template<int Size, int N = Size - 1>
    struct recursive_invoke
    {
        template<class Tuple, class Input>
        static auto process(Tuple&& tuple, Input&& in)
        {
            return recursive_invoke<Size, N - 1>::process(tuple, std::invoke(std::get<Size - N - 1>(tuple), std::forward<Input>(in)));
        }
    };

    template<int Size>
    struct recursive_invoke<Size, 0>
    {
        template<class Tuple, class Input>
        static auto process(Tuple&& tuple, Input&& in)
        {
            return std::invoke(std::get<Size - 1>(tuple), std::forward<Input>(in));
        }
    };

    template<class... Functions>
    struct task_chain
    {
        static constexpr auto size() { return sizeof...(Functions); }

        template<unsigned int N>
        using function_at = typename std::tuple_element_t<N, std::tuple<Functions...>>;

        task_chain(Functions... fns) : functions_{ fns... }
        {
        }

        template<class Input>
        auto process(Input&& in)
        {
            return recursive_invoke<sizeof...(Functions)>::process(functions_, std::forward<Input>(in));
        }

        std::tuple<Functions...> functions_;
    };







    template<class F, class Executor>
    class task
    {
    public:
        using return_type = typename function_trait<F>::return_type;
        using function_type = typename function_trait<F>::function_type;


        task(F callable, Executor& ex)
            : executor_{ ex }
            , function_{  std::move(callable) }
        {}

        task(const task&) = delete;

        task(task&&) noexcept = default;
        task& operator=(task&&) noexcept = default;

        auto fork()
        {
            nes::task fork_task(function_, executor_);
            return fork_task;
        }

        template<class... Args>
        auto operator()(Args&&... args)
        {
            auto value = function_(std::forward<Args>(args)...);
            promise_.set_value(value);
            return value;
        }

        template<class T>
        auto set_value(T v)
        {
            promise_.set_value(v);
        }

        void reset()
        {
            promise_ = std::promise<return_type>{};
        }

        auto get_future() { return promise_.get_future(); }

        Executor& context() { return executor_; }

    private:
        Executor& executor_;
        std::promise<return_type> promise_;
        F function_;
    };
} // nse

#endif // TASK_HPP_NES
