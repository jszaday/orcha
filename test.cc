#include "orcha.hh"
#include <iostream>
#include <numeric>

class test {
public:
  int i;

  test(int _i) : i(_i) { }

  // std::tuple<int> produce(void) {
  //   return std::forward_as_tuple(i);
  // }

  int a(void) {
    return i;
  }

  int b(void) {
    return i;
  }

  void c(int a, int b) {
    std::cout << i << " received a message from " << a << " and " << b << std::endl;
  }
};

int main(void) {
  const int n = 10;
  std::vector<test> arr;
  // producer and consumer index sets
  std::vector<int> asi(n);
  std::vector<int> bsi(n);
  std::vector<int> csi(n);
  // initialize index sets
  std::iota(csi.begin(), csi.end(), 0);
  std::transform(csi.begin(), csi.end(), asi.begin(), [n] (int i) {
    return (i - 1 + n) % n;
  });
  std::transform(csi.begin(), csi.end(), bsi.begin(), [n] (int i) {
    return (i + 1) % n;
  });
  // initialize proto-chare array
  std::for_each(csi.begin(), csi.end(), [&arr](int i) mutable {
    arr.emplace_back(i);
  });
  // map the produce/consume functions over the index sets
  auto as = orcha::map(arr, asi, orcha::bind(&test::a));
  auto bs = orcha::map(arr, bsi, orcha::bind(&test::b));
  auto cs = orcha::map(arr, csi, orcha::bind(&test::c));
  // then map the producers onto the consumers
  orcha::strate(orcha::zip2(as, bs), cs);
  // then reduce on the A producers
  // (an explicit cast is currently required for std::functional built-ins)
  auto plus = static_cast<std::function<int(int, int)>>(std::plus<int>());
  std::cout << "reduction evaluated to " << orcha::reduce(as, 0, plus) << std::endl;
}
