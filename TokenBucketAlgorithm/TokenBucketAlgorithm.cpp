//Copyright(c) 2023 Paulo F. Gomes
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>

/// <summary>
/// Example implementation of Token Bucket algorithm.
/// https://en.wikipedia.org/wiki/Token_bucket
/// </summary>
class TokenBucket {
public:
    /// <summary>
    /// TokenBucket constructor.
    /// </summary>
    /// <param name="size">Maximum number of tokens in bucket</param>
    /// <param name="refillFrequencySeconds">How frequently the bucket is refilled (seconds)</param>
    TokenBucket(int size, int refillFrequencySeconds) :
        max_counter_(size),
        counter_(size),
        refiller_(&TokenBucket::refill, this, refillFrequencySeconds){}

    ~TokenBucket() {
        {
            std::lock_guard lock(counter_mutex_);
            exit_ = true;
        }
        condition_variable_.notify_one();
        refiller_.join();
    }

    /// <summary>
    /// Try to make a request.
    /// </summary>
    /// <returns>True if there are tokens available (request should be successful).</returns>
    bool request() {
        bool result = false;
        {
            std::lock_guard lock(counter_mutex_);
            if (counter_ != 0) {
                --counter_;
                result = true;
            }
        }
        return result;
    }
private:
    int max_counter_;
    int counter_;
    std::mutex counter_mutex_;
    std::thread refiller_;
    bool exit_ = false;
    std::condition_variable condition_variable_;

    void refill(int refillFrequencySeconds) {
        using namespace std::chrono_literals;
        while (true) {
            {
                std::unique_lock lock(counter_mutex_);
                if (exit_ || condition_variable_.wait_for(lock, refillFrequencySeconds * 1s, [this] {return exit_; })) {
                    return;
                }
                counter_ = max_counter_;
            }
            std::cout << std::endl << "Refill of " << max_counter_ << std::endl << ">";
        }
    }
};

/// <summary>
/// Creates a TokenBucket using user input and waits on requests,
/// represented as user inputted strings. Strings can be empty.
/// If the request is "q" (quit) the program ends, otherwise
/// check if the request would be sucessful according to the
/// TokenBucket.
/// </summary>
int main()
{
    int tokenBucketSize;
    int tokenBucketRefillFrequencySeconds;
    std::cout << "Please enter token bucket size: ";
    std::cin >> tokenBucketSize;
    std::cout << "Please enter token bucket refill frequency (seconds): ";
    std::cin >> tokenBucketRefillFrequencySeconds;
    // Because of std::getline below
    std::cin.ignore(1);

    TokenBucket tokenBucket(tokenBucketSize, tokenBucketRefillFrequencySeconds);

    std::string request;
    while (true) {
        std::cout << "Please enter request: " << std::endl;
        std::cout << ">";
        std::getline(std::cin, request);
        if (request == "q") {
            break;
        }
        bool result = tokenBucket.request();
        std::cout << "Request " << request << " " << (result ? "succeeded" : "dropped") << std::endl;
    }
}