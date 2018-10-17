#include "core.hh"

#include <random>
#include <iostream>

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(1, 6);

class test {
  int i_;
public:
  test(int i) : i_(i) { }

  int a(void) {
    return i_;
  }

  int b(int a) {
    // std::this_thread::sleep_for(std::chrono::seconds(dis(gen)));
    return a * 2;
  }

  void c(int b) {
    std::cout << i_ << " received the value " << b << std::endl;
  }
};

int main(int argc, char **argv) {
  // Initialize the library
  orcha::comm::initialize(argc, argv);
  const int n = 10;
  // Create the index space (0, 1, 2, ...)
  auto is  = orcha::index_space(n);
  // Create the distributed array
  auto arr = orcha::distribute<test>(orcha::automap(is));
  // Register the producer and consumer
  auto a = orcha::register_function(arr, &test::a);
  auto b = orcha::register_function(arr, &test::b);
  auto c = orcha::register_function(arr, &test::c);
  // Run the orchestration code
  auto cis = orcha::map<orcha::id_t>(is, [&n] (orcha::id_t i) -> orcha::id_t {
    return (i + 1) % n;
  });
  auto as = orcha::strate(a, is);
  auto bs = orcha::strate(b, cis, as);
  orcha::strate(c, is, bs);
  // Wait until all of the nodes have finished
  orcha::comm::barrier(orcha::comm::global());
  // Then finalize the library
  orcha::comm::finalize();
  return 0;
}
