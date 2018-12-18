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

    auto f4 = [](int a, int b) -> int { std::cout << "\nCALL F4"; return a + b; };
    auto f5 = []() -> double { std::cout << "\nCALL F5"; return 5 * 1000; };
    auto f6 = []() -> std::string { std::cout << "\nCALL F6"; return "result : " + std::to_string(8); };

    /*
    nes::thread_pool_executor ex(5);
    auto r4 = ex.post(f4, 5, 2);
    auto r5 = ex.post(f5);
    auto r6 = ex.post(f6);

    std::cout << "\nRESULTS : " << std::endl;
    std::cout << "\n" << r4.get();
    std::cout << "\n" <<  r5.get();
    std::cout << "\n" <<  r6.get();
     */

    nes::thread_pool_executor tp_ex(5);
    nes::thread_pool_executor tp_ex15(15);

    //nes::task t1{ f1, tp_ex };
    //nes::task t2{ f2, tp_ex };

/*
    nes::static_pipeline sp(std::make_tuple(nes::task{f1, tp_ex}, nes::task{f2, tp_ex}));
    sp.push(1);
    sp.push(5);


    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto output = sp.output();
    std::cout << "\n" << "RESULT" << output.front();
    output.pop();
    std::cout << "\n" << "RESULT" << output.front();
*/





    std::cout << "make pipeline";
    nes::pipeline p{ nes::pipeline_step{f1}, nes::pipeline_step{f2}, nes::pipeline_step{f3} };
    p.push(2);
    p.push(1);

    std::this_thread::sleep_for(std::chrono::seconds(2));


    std::cout << "\n" << "RESULTS : ";
    for (auto item :  p.output())
    {
        std::cout << "\n" << "item : " << item;
    }
}