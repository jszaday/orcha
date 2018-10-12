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
    std::this_thread::sleep_for(std::chrono::seconds(dis(gen)));
    return i_;
  }

  void b(int a) {
    std::cout << i_ << " received a message from " << a << std::endl;
  }
};

int main(void) {
  const int n = 10;
  // Create the index space (0, 1, 2, ...)
  auto is  = orcha::index_space(n);
  // Determine which elements reside on the local node
  auto map = orcha::mapping::automatic(is);
  // Create the distributed array
  auto arr = orcha::distribute<test>(map, is);
  // Register the producer and consumer
  auto a   = orcha::register_function(arr, &test::a);
  auto b   = orcha::register_function(arr, &test::b);
  // Map the as onto their right neighbors
  auto as = orcha::strate(a, is);
  orcha::strate(b, orcha::map<orcha::id_t>(is, [&n] (orcha::id_t i) -> orcha::id_t {
    return (i + 1) % n;
  }), as);
  return 0;
}
