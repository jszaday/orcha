#include "core.hh"
#include <cstdlib>

class life {
    bool* grid;
    size_t x, y, n;

public:
    life(std::tuple<size_t, size_t> xy, const size_t n_)
    {
        grid = new bool[(n_ + 1) * (n_ + 1)];
        x = std::get<0>(xy);
        y = std::get<1>(xy);
        n = n_;
    }

    ~life()
    {
        delete grid;
    }

    // grid[1][1...n]
    std::vector<bool> produce_north()
    {
        return std::vector<bool>(grid + n + 1, grid + 2 * n);
    }

    // grid[1...n][1]
    std::vector<bool> produce_east()
    {
        std::vector<bool> ret(n);
        for (auto i = 0; i < n; i++) {
            ret[i] = grid[i * n + 1];
        }
        return ret;
    }

    // grid[n - 2][1...n]
    std::vector<bool> produce_south()
    {
        return std::vector<bool>(grid + (n - 2) * (n + 1) + 1, grid + (n - 1) * (n + 1));
    }

    // grid[1...n][n - 2]
    std::vector<bool> produce_west()
    {
        std::vector<bool> ret(n);
        for (auto i = 0; i < n; i++) {
            ret[i] = grid[(i + 2) * n - 2];
        }
        return ret;
    }

    // update boundaries and run game of life
    // north, east, south, west
    void consume(std::tuple<std::vector<bool>, std::vector<bool>, std::vector<bool>, std::vector<bool>> ghosts)
    {
        for (auto i = 0; i < n; i++) {
            grid[i] = std::get<0>(ghosts)[i];
            grid[i * (n + 1)] = std::get<1>(ghosts)[i];
            grid[i + (n - 1) * (n + 1)] = std::get<2>(ghosts)[i];
            grid[(i + 1) * (n + 1) - 1] = std::get<3>(ghosts)[i];
        }

        auto prev = grid;
        auto next = new bool[(n + 1) * (n + 1)];

        for (int a = 1; a < (n + 1); a++) {
            for (int b = 1; b < (n + 1); b++) {
                int alive = 0;
                for (int c = -1; c < 2; c++) {
                    for (int d = -1; d < 2; d++) {
                        if (!(c == 0 && d == 0)) {
                            if (prev[(a + c) * (n + 1) + (b + d)]) {
                                ++alive;
                            }
                        }
                    }
                }
                if (alive < 2) {
                    next[a * (n + 1) + b] = false;
                } else if (alive == 3) {
                    next[a * (n + 1) + b] = true;
                } else if (alive > 3) {
                    next[a * (n + 1) + b] = false;
                }
            }
        }

        delete prev;
        grid = next;
    }
};

int main(int argc, char** argv)
{
    // Initialize the library
    orcha::comm::initialize(argc, argv);
    // Get the arguments
    const int sz = argc >= 2 ? std::atoi(argv[1]) : 256;
    const int it = argc >= 3 ? std::atoi(argv[2]) : 8;
    const int rk = orcha::comm::rank(orcha::comm::global());
    const int n = sqrt(orcha::comm::size(orcha::comm::global()));
    // Create the index space (0, 1, 2, ...)
    auto xs = orcha::index_space(n);
    auto is = xs * xs;
    // Create the distributed array
    auto arr = orcha::distribute<life>(orcha::automap(is), sz / n);
    // Register the producers and consumers
    auto pn = orcha::register_function(arr, &life::produce_north);
    auto pe = orcha::register_function(arr, &life::produce_east);
    auto ps = orcha::register_function(arr, &life::produce_south);
    auto pw = orcha::register_function(arr, &life::produce_west);
    auto c = orcha::register_function(arr, &life::consume);
    // Conway's Game of Life
    if (rk == 0) {
        std::cout << "Running " << it << " iteration(s) of life on " << n * n << " PE(s), with a " << sz << " x " << sz << " grid." << std::endl;
        std::cout << "Each PE is managing a " << sz / n << " x " << sz / n << " chunk." << std::endl;
    }
    auto nis = is.map([n](const std::tuple<size_t, size_t>& xy) {
        auto x = std::get<0>(xy), y = std::get<1>(xy);
        return std::make_tuple(x, (n + y - 1) % n);
    });
    auto eis = is.map([n](const std::tuple<size_t, size_t>& xy) {
        auto x = std::get<0>(xy), y = std::get<1>(xy);
        return std::make_tuple((n + x - 1) % n, y);
    });
    auto sis = is.map([n](const std::tuple<size_t, size_t>& xy) {
        auto x = std::get<0>(xy), y = std::get<1>(xy);
        return std::make_tuple(x, (y + 1) % n);
    });
    auto wis = is.map([n](const std::tuple<size_t, size_t>& xy) {
        auto x = std::get<0>(xy), y = std::get<1>(xy);
        return std::make_tuple((x + 1) % n, y);
    });
    for (int i = 0; i < it; i++) {
        if (rk == 0) {
            std::cout << "Iteration " << (i + 1) << " of " << it << std::endl;
        }
        auto ns = orcha::strate(pn, nis);
        auto es = orcha::strate(pe, eis);
        auto ss = orcha::strate(ps, sis);
        auto ws = orcha::strate(pw, wis);
        auto ts = orcha::zip4(ns, es, ss, ws);
        orcha::strate(c, is, ts);
        orcha::comm::barrier(orcha::comm::global());
    }
    // Then finalize the library
    orcha::comm::finalize();
    return 0;
}
