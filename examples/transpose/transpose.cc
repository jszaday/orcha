#include "core.hh"
#include <cstdlib>

// std::mutex k_cout_lock_;

class transpose {
    size_t x, y;
    float a, a_t;

public:
    transpose(std::tuple<size_t, size_t> idx, int n)
    {
        x = std::get<0>(idx);
        y = std::get<1>(idx);
        a = a_t = x * n + y;
        // std::lock_guard<std::mutex> guard(k_cout_lock_);
        std::cout << "a[" << x << ", " << y << "] = " << a << std::endl;
    }

    float produce(void)
    {
        return a;
    }

    void consume(float a)
    {
        a_t = a;
    }

    void print()
    {
        // std::lock_guard<std::mutex> guard(k_cout_lock_);
        std::cout << "a[" << x << ", " << y << "]"
                  << " = " << a_t << std::endl;
    }
};

int main(int argc, char** argv)
{
    // Initialize the library
    orcha::comm::initialize(argc, argv);
    // Get the arguments
    const int n = argc >= 2 ? std::atoi(argv[1]) : 2;
    // Create the index space (0, 1, 2, ...)
    auto xs = orcha::index_space(n);
    auto is = xs * xs;
    // Create the distributed array
    auto arr = orcha::distribute<transpose>(orcha::automap(is), n);
    // Register the producers and consumers
    auto p = orcha::register_function(arr, &transpose::produce);
    auto c = orcha::register_function(arr, &transpose::consume);
    // Transpose Algorithm
    for (int i = 0; i < (n - 1); i++) {
        auto isa = orcha::singleton(i) * orcha::index_space(i + 1, n);
        auto isb = isa.map([](const std::tuple<size_t, size_t>& xy) {
            auto x = std::get<0>(xy), y = std::get<1>(xy);
            return std::make_tuple(y, x);
        });
        orcha::strate(c, isb, orcha::strate(p, isa));
        orcha::strate(c, isa, orcha::strate(p, isb));
    }
    orcha::comm::barrier(orcha::comm::global());
    orcha::call_local(arr, is, &transpose::print);
    // Then finalize the library
    orcha::comm::finalize();
    return 0;
}
