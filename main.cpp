#include "thqueue.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>

/// @brief  bounded queue
thqueue<std::string> q(1000);
/// @brief use atomic bool to stop threads
std::atomic<bool> stop = false;
using namespace std::chrono_literals;

/// @brief funtion emulate consumer thread
void *consumer_thread()
{
    std::cout << "Consumer pid:" << std::this_thread::get_id() << " started" << std::endl;
    for (;;) {
        std::string data;
        if (q.try_get(&data)) {
            const char *buffer = data.c_str();
            std::cout << "Consumer get data : " << buffer << std::endl;
            std::this_thread::sleep_for(10ns);
        } else {
            if (stop)
                break;
        }
    }
    std::cout << "Consumer pid:" << std::this_thread::get_id() << " stopped" << std::endl;
    return 0;
}

/// @brief funtion emulate data for queue
std::string getdata()
{
    std::stringstream ss;
    static std::atomic<int> seq = 0;
    std::mutex mt;
    std::unique_lock<std::mutex> lock(mt);
    ss << "seq = " << (seq++) <<" from pid "<<std::this_thread::get_id();
    lock.unlock();
    return ss.str();
}

/// @brief funtion emulate producer thread
void *producer_thread()
{
    std::cout << "Producer pid:" << std::this_thread::get_id() << " started" << std::endl;

    while (!stop) {
        q.try_put(getdata());
        std::this_thread::sleep_for(10ns);
    }
    std::cout << "Producer pid:" << std::this_thread::get_id() << " stopped" << std::endl;

    return 0;
}
/// @file
/// @brief main function for testing thread queue
/// @author btv<example@example.com>
int main(int argc, char **argv)
{
    (void) argc;
    int n = argv[1] ? atoi(argv[1]) : 5;
    if (n < 0) {
        n = -n;
    }

    std::cout << "test 1 consumer and " << n << " producers" << std::endl;
    std::vector<std::thread> threads;
    char *in = NULL;
    size_t sz;

/// @brief one consumer 
    threads.emplace_back(std::thread(&consumer_thread));

/// @brief many producers
    for (int i = 0; i < n; i++)
        threads.emplace_back(std::thread(&producer_thread));

    while ((puts("Enter (for info or ^D for exit)"), getline(&in, &sz, stdin)) > 0) {
    }
    stop = true;
    std::cout << "Wait all threads" << std::endl;

    for (auto &thread : threads) {
        if (thread.joinable())
            thread.join();
    }

    return 0;
}
