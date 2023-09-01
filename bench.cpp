#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <barrier>

std::chrono::high_resolution_clock m_clock;

uint64_t nanoseconds()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(m_clock.now().time_since_epoch()).count();
}

void set_affinity(int cpu)
{
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_t current_thread = pthread_self();

    if (int rc = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset))
    {
        fprintf(stderr, "%s\n", strerror(rc));
        exit(1);
    }
    pthread_yield();
#endif
}

constexpr std::size_t numberOfSamples = 10000;
constexpr std::size_t numberOperationPerSample = 10000;
constexpr std::size_t writerCpu = 1;
constexpr std::size_t readerCpu = 2;

void runBenchmark()
{
    std::vector<uint64_t> results;
    results.reserve(numberOfSamples);
    std::barrier sync_point(2);
    std::atomic_bool flag = false;

    auto writer = [&]()
    {
        set_affinity(writerCpu);
        for (size_t i = 0; i < numberOfSamples; ++i)
        {
            sync_point.arrive_and_wait();
            for (size_t j = 0; j < numberOperationPerSample; j++)
            {
                while (flag.load(std::memory_order_relaxed) != false)
                    ;
                flag.store(true, std::memory_order_relaxed);
            }
        }
    };
    auto reader = [&]()
    {
        set_affinity(readerCpu);
        for (size_t i = 0; i < numberOfSamples; ++i)
        {
            sync_point.arrive_and_wait();
            uint64_t start = nanoseconds();
            for (size_t j = 0; j < numberOperationPerSample; j++)
            {
                while (flag.load(std::memory_order_relaxed) != true)
                    ;
                flag.store(false, std::memory_order_relaxed);
            }
            uint64_t end = nanoseconds();
            results.push_back(end - start);
        }
    };

    auto loadStoreLatency = [&flag]()
    {
        flag = true;
        set_affinity(readerCpu);
        std::vector<uint64_t> latencies;
        latencies.reserve(numberOfSamples);
        for (size_t i = 0; i < numberOfSamples; ++i)
        {
            uint64_t start = nanoseconds();
            for (size_t j = 0; j < numberOperationPerSample; j++)
            {
                while (flag.load(std::memory_order_relaxed) != true)
                    ;
                flag.store(true, std::memory_order_relaxed);
            }
            uint64_t end = nanoseconds();
            latencies.push_back(end - start);
        }
        std::sort(begin(latencies), end(latencies));
        return latencies[latencies.size() / 2] / numberOperationPerSample;
    };

    std::thread t1(writer);
    std::thread t2(reader);
    t1.join();
    t2.join();

    auto correction = loadStoreLatency();
    std::sort(begin(results), end(results));
    std::cout   << "median cache syncronazation latency: (" 
                << results[results.size() / 2] / (numberOperationPerSample * 2) 
                << "-" << correction << ") ns\n";
}

int main(int argc, char **argv)
{
    runBenchmark();
};
