#include "core.hh"
#include <cstdlib>

std::mutex k_cout_lock_;

class lu {
    float a, l, u;
    size_t x, y;

public:
    lu(std::tuple<size_t, size_t> idx, int n)
    {
        x = std::get<0>(idx);
        y = std::get<1>(idx);
        a = x * n + y;
        l = u = 0;
        std::lock_guard<std::mutex> guard(k_cout_lock_);
        std::cout << "a[" << x << ", " << y << "] = " << a << std::endl;
    }

    float produce_l(void)
    {
        return a;
    }

    float produce_u(void)
    {
        return u;
    }

    float multiply(std::tuple<float, float> lu)
    {
        return std::get<0>(lu) * std::get<1>(lu);
    }

    void utri(float sum)
    {
        u = a - sum;
    }

    void ltri(std::tuple<float, float> sum_u)
    {
        if (x == y) {
            l = 1;
        } else {
            l = (a - std::get<0>(sum_u)) / std::get<1>(sum_u);
        }
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
    // Create the distributed array
    auto arr = orcha::distribute<lu>(orcha::automap(is), n);
    // Register the producers and consumers
    auto l = orcha::register_function(arr, &lu::produce_l);
    auto u = orcha::register_function(arr, &lu::produce_u);

    auto ls = orcha::strate(l, is);
    float lsum = orcha::reduce(ls, [](float x, float y) {
      return x + y;
    }, 0.0f);
    std::cout << "sum of all values " << lsum << std::endl;
    // Then finalize the library
    orcha::comm::finalize();
    return 0;
}
