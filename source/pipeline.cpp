#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include <tuple>

#include <thread_pool.hpp>
#include <pipeline_executor.hpp>
#include <pipeline.hpp>
#include <future>

template<class... Args>
auto make_unique_pipeline(Args&&... args)
{
    return std::make_unique<nes::pipeline<Args...>>(std::forward<Args>(args)...);
}

struct zeta
{
    inline static auto f1 = [](int n) -> double { return 1 + n; };
    inline static auto f2 = [](double n) -> double { return n * 10; };
    inline static auto f3 = [](double n) -> std::string { return "str_" + std::to_string(n); };

    using pipeline_type = nes::basic_pipeline<int, std::string>;

    zeta()
        : ex1_{ 10 }
        , ex2_{ 5 }
        , pipeline_{ make_unique_pipeline(
            [](int n) -> double { return 1 + n; }
            , nes::pipeline_step{f2, ex1_}
            , nes::pipeline_step{f3, ex2_}
        )}
    {}

    void test()
    {
        pipeline_->push(2);
        pipeline_->push(1);
        pipeline_->push(2);
        pipeline_->push(5);

        //pipeline_->cancel(1);

        std::this_thread::sleep_for(std::chrono::seconds(2));


        std::cout << "\n" << "RESULTS : ";
        for (auto item :  pipeline_->output())
        {
            std::cout << "\n" << "item : " << item;
        }
    }

    nes::thread_pool_executor ex1_;
    nes::thread_pool_executor ex2_;

    std::unique_ptr<pipeline_type> pipeline_;
};

int main()
{
    zeta z;

    z.test();



}