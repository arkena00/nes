#include <benchmark/benchmark.h>
#include <thread_pool.hpp>
#include <pipeline.hpp>
#include <random>
#include <algorithm>
#include <iostream>

static constexpr auto pushed_data = 25;

inline static auto f1 = [](std::vector<int> v) -> std::vector<int> { std::for_each(v.begin(), v.end(), [](int& n) { n / 3 + 1; } ); return v; };
inline static auto f2 = [](std::vector<int> v) -> std::vector<int> { std::sort(v.begin(), v.end()); return v; };
inline static auto f3 = [](std::vector<int> v) -> std::vector<std::string>
{
    std::vector<std::string> output;
    output.resize(v.size());
    for (auto item : v)
    {
        output.push_back("str_" + std::to_string(item));
    }
    return output;
};

inline static auto f4 = [](std::vector<int> v) -> std::vector<std::string>
{
    auto r1 = f1(v);
    auto r2 = f2(std::move(r1));
    auto r3 = f3(std::move(r2));
    return r3;
};

auto make_data(uint64_t n)
{
    std::vector<int> data;
    data.resize(n);
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> dis(1, 100000);

    std::generate(data.begin(), data.end(), std::bind(dis, generator));
    return data;
}

auto nes_pipeline = [](benchmark::State& state)
{
    nes::thread_pool_executor ex1_;
    nes::thread_pool_executor ex2_;

    //std::vector<int> data = make_data(state.range(0));
    std::vector<std::vector<int>> datas;
    for (int i = 0; i != pushed_data; ++i)
    {
        datas.push_back(make_data(state.range(0)));
    }

    nes::pipeline p{f1, f2, f3 };

    while(state.KeepRunning())
    {
        for (auto data : datas)
        {
            p.push(data);
        }

        p.wait();
    }
};

auto nes_pipeline_thread_pool = [](benchmark::State& state)
{
    nes::thread_pool_executor ex1_( state.range(1) );

    //std::vector<int> data = make_data(state.range(0));
    std::vector<std::vector<int>> datas;
    for (int i = 0; i != pushed_data; ++i)
    {
        datas.push_back(make_data(state.range(0)));
    }

    nes::pipeline p{ nes::pipeline_step{f1, ex1_}, nes::pipeline_step{f2, ex1_}, nes::pipeline_step{f3, ex1_} };

    while(state.KeepRunning())
    {
        for (auto data : datas)
        {
            p.push(data);
        }

        p.wait();
    }
};

auto nes_no_pipeline = [](benchmark::State& state)
{
    //std::vector<int> data = make_data(state.range(0));
    std::vector<std::vector<int>> datas;
    for (int i = 0; i != pushed_data; ++i)
    {
        datas.push_back(make_data(state.range(0)));
    }

    while(state.KeepRunning())
    {
        for (auto data : datas)
        {
            auto r1 = f1(data);
            auto r2 = f2(std::move(r1));
            auto r3 = f3(std::move(r2));
        }
    }
};

auto nes_no_pipeline_thread_pool = [](benchmark::State& state)
{
    nes::thread_pool_executor ex( state.range(1) );

    std::vector<std::future<std::vector<std::string>>> results;
    //std::vector<int> data = make_data(state.range(0));
    std::vector<std::vector<int>> datas;
    for (int i = 0; i != pushed_data; ++i)
    {
        datas.push_back(make_data(state.range(0)));
    }
    results.reserve(datas.size());

    while(state.KeepRunning())
    {
        for (auto& data : datas)
        {
            std::future<std::vector<std::string>> f = ex.post_fut(f4, data);
            results.push_back(std::move(f));
        }

        for (auto& result : results)
        {
            result.wait();
        }
    }
};


int main(int argc, char** argv)
{

    benchmark::RegisterBenchmark("nes_no_pipeline", nes_no_pipeline)->Unit(benchmark::kMillisecond)->RangeMultiplier(10)->Range(1'000, 1'00'000);
    benchmark::RegisterBenchmark("nes_pipeline", nes_pipeline)->Unit(benchmark::kMillisecond)->RangeMultiplier(10)->Range(1'000, 1'00'000);
    benchmark::RegisterBenchmark("nes_pipeline_thread_pool", nes_pipeline_thread_pool)->Unit(benchmark::kMillisecond)
    ->Args({1'000, 2})
    ->Args({1'000, 4})
    ->Args({1'000, 8})
    ->Args({10'000, 2})
    ->Args({10'000, 4})
    ->Args({1'00'000, 2})
    ->Args({1'00'000, 4})
    ->Args({1'00'000, 8});

    benchmark::RegisterBenchmark("nes_no_pipeline_thread_pool", nes_no_pipeline_thread_pool)->Unit(benchmark::kMillisecond)
    ->Args({1'000, 2})
    ->Args({1'000, 4})
    ->Args({10'000, 2})
    ->Args({10'000, 4})
    ->Args({1'00'000, 2})
    ->Args({1'00'000, 4})
    ->Args({1'00'000, 8});

    //auto b2 = benchmark::RegisterBenchmark("testing", nse_test)->Unit(benchmark::kMicrosecond);


    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();


    return 0;
}
