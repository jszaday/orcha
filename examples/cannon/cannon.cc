#include "core.hh"
#include <cstdlib>

std::mutex k_cout_lock_;

class cannon {
    float c;
    float a, b;
    size_t x, y;

public:
    cannon(std::tuple<size_t, size_t> idx, int n)
    {
        x = std::get<0>(idx);
        y = std::get<1>(idx);
        auto k = (x + y) % n;
        a = x * n + k;
        b = k * n + y;
        c = 0;
        std::lock_guard<std::mutex> guard(k_cout_lock_);
        std::cout << "(a, b)[" << x << ", " << y << "] = " << (x * n + y) << std::endl;
    }

    float produce_a(void)
    {
        return a;
    }

    float produce_b(void)
    {
        return b;
    }

    void multiply(std::tuple<float, float> ab)
    {
        c += a * b;
        a = std::get<0>(ab);
        b = std::get<1>(ab);
        std::lock_guard<std::mutex> guard(k_cout_lock_);
        std::cout << "c[" << x << ", " << y << "]"
                  << " = " << c << std::endl;
    }
};

template <typename T, typename U>
std::ostream& operator<<(std::ostream& o, const std::tuple<T, U>& p)
{
    return o << "(" << std::get<0>(p) << ", " << std::get<1>(p) << ")";
}

int main(int argc, char** argv)
{
    // Initialize the library
    orcha::comm::initialize(argc, argv);
    // Get the arguments
    const int n = argc >= 2 ? std::atoi(argv[1]) : 2;
    // Create the index space (0, 1, 2, ...)
    auto xs = orcha::index_space(n);
    auto is = xs * xs;
    auto ais = is.map([n](const std::tuple<size_t, size_t>& i) {
        auto x = std::get<0>(i), y = std::get<1>(i);
        return std::make_tuple(x, (y - 1 + n) % n);
    });
    auto bis = is.map([n](const std::tuple<size_t, size_t>& i) {
        auto x = std::get<0>(i), y = std::get<1>(i);
        return std::make_tuple((x - 1 + n) % n, y);
    });
    std::cout << ais.to_string() << std::endl;
    // Create the distributed array
    auto arr = orcha::distribute<cannon>(orcha::automap(is), n);
    // Register the producers and consumers
    auto a = orcha::register_function(arr, &cannon::produce_a);
    auto b = orcha::register_function(arr, &cannon::produce_b);
    auto c = orcha::register_function(arr, &cannon::multiply);
    // Cannon's Algorithm
    for (int i = 0; i < n; i++) {
        std::cout << "iteration " << i + 1 << " out of " << n << std::endl;
        auto as = orcha::strate(a, ais);
        auto bs = orcha::strate(b, bis);
        auto abs = orcha::zip2(as, bs);
        orcha::strate(c, is, abs);
        orcha::comm::barrier(orcha::comm::global());
    }
    // orcha::comm::barrier(orcha::comm::global());
    // Then finalize the library
    orcha::comm::finalize();
    return 0;
}
