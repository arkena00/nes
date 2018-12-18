#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include <tuple>

#include <thread_pool.hpp>
#include <pipeline_executor.hpp>
#include <pipeline.hpp>
#include <future>


int main()
{
    auto f1 = [](int n) -> double { return 1 + n; };
    auto f2 = [](double n) -> double { return n * 10; };
    auto f3 = [](double n) -> std::string { return "str_" + std::to_string(n); };

    nes::pipeline p{ f1, nes::pipeline_step{f2}, nes::pipeline_step{f3} };
    p.push(2);
    p.push(1);

    std::this_thread::sleep_for(std::chrono::seconds(2));


    std::cout << "\n" << "RESULTS : ";
    for (auto item :  p.output())
    {
        std::cout << "\n" << "item : " << item;
    }
}