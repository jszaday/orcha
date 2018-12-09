#include "core.hh"
#include <cstdlib>

// std::mutex k_cout_lock_;

class prefix {
    size_t x;
    float a, s;

public:
    prefix(size_t idx)
        : x(idx)
        , a(idx + 1)
        , s(idx + 1)
    {
        // std::lock_guard<std::mutex> guard(k_cout_lock_);
        std::cout << "a[" << x << "] = " << a << std::endl;
    }

    float produce(void)
    {
        return s;
    }

    void consume(float a)
    {
        s += a;
    }

    void print()
    {
        // std::lock_guard<std::mutex> guard(k_cout_lock_);
        std::cout << "s[" << x << "] = " << s << std::endl;
    }
};

int main(int argc, char** argv)
{
    // Initialize the library
    orcha::comm::initialize(argc, argv);
    // Get the arguments
    const int n = argc >= 2 ? std::atoi(argv[1]) : 8;
    // Create the index space (0, 1, 2, ...)
    auto is = orcha::index_space(n);
    // Create the distributed array
    auto arr = orcha::distribute<prefix>(orcha::automap(is));
    // Register the producers and consumers
    auto p = orcha::register_function(arr, &prefix::produce);
    auto c = orcha::register_function(arr, &prefix::consume);
    // Prefix-Sum Algorithm
    for (auto i = 0; i <= (size_t)log2(n); i++) {
        auto js = orcha::index_space(1 << i, n);
        auto ks = js.map([i](size_t j) {
            return j - (1 << i);
        });
        orcha::strate(c, js, orcha::strate(p, ks));
        orcha::barrier();
    }
    orcha::call_local(arr, is, &prefix::print);
    // Then finalize the library
    orcha::comm::finalize();
    return 0;
}
