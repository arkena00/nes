#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include <tuple>

#include <thread_pool.hpp>
#include <pipeline_executor.hpp>
#include <future>


int main()
{
    auto f1 = [](int n) -> int { return 4 + n; };
    auto f2 = [](int n) -> double { return n * 1000; };
    auto f3 = [](double n) -> std::string { return "result : " + std::to_string(n); };

    auto f4 = [](int a, int b) -> int { std::cout << "\nCALL F4"; return a + b; };
    auto f5 = []() -> double { std::cout << "\nCALL F5"; return 5 * 1000; };
    auto f6 = []() -> std::string { std::cout << "\nCALL F6"; return "result : " + std::to_string(8); };

    nes::thread_pool_executor ex(5);

    //nes::task {f1, 4, 5};

    auto r4 = ex.post(f4, 5, 2);
    auto r5 = ex.post(f5);
    auto r6 = ex.post(f6);


    std::cout << "\nRESULTS : " << std::endl;
    std::cout << "\n" << r4.get();
    std::cout << "\n" <<  r5.get();
    std::cout << "\n" <<  r6.get();

    nes::thread_pool_executor tp_ex(5);
    nes::thread_pool_executor tp_ex15(15);

    nes::task t1{ f1, tp_ex };
    nes::task t3{ f1, tp_ex };

    nes::task_chain chain( f1, f2, f3 );

    nes::static_pipeline sp(chain);
    sp.post(5);
    sp.post(9);


    //ex.post(wrap(task1, tp_ex.executor()));
    //ex.post(wrap(task2, tp_ex15.executor()));


    //std::cout << "result : " << pt.process(1);
}