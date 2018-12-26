#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include <tuple>

#include <thread_pool.hpp>
#include <pipeline_executor.hpp>
#include <pipeline.hpp>
#include <future>

void test()
{
    auto f1 = [](int n) { return  n; };

    nes::thread_pool_executor ex( 4 );

    std::vector<std::future<int>> results;
    std::vector<int> datas;
    for (int i = 0; i != 5; i++)
    {
        datas.push_back(i);
    }
    results.reserve(datas.size());

    for (auto& data : datas)
    {
        auto f = ex.post_fut(f1, data);
        results.push_back(std::move(f));
    }

    for (auto& result : results)
    {
        result.wait();
        std::cout << "_" << result.get();
    }
}

int main()
{
    auto f1 = [](int n) -> double { return 1 + n; };
    auto f2 = [](double n) -> double { return n * 10; };
    auto f3 = [](double n) -> std::string { return "str_" + std::to_string(n); };

    auto f4 = [](int a, int b) -> int { std::cout << "\nCALL F4"; return a + b; };
    auto f5 = []() -> double { std::cout << "\nCALL F5"; return 5 * 1000; };
    auto f6 = []() -> std::string { std::cout << "\nCALL F6"; return "result : " + std::to_string(8); };




    test();

    /*
    nes::thread_pool_executor tp_ex(5);
    nes::thread_pool_executor tp_ex15(15);

    nes::inline_executor ie;

    nes::task t1{ f1, tp_ex };
    nes::task t2{ f2, ie };*/

}