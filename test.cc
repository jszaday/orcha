#include <orcha.hh>
#include <iostream>
#include <numeric>

template <typename K, typename V>
class test_array {
  using vect_t = std::vector<V>;
  vect_t& arr;
public:
  test_array(vect_t& _arr) : arr(_arr) { }

  V& operator() (const K &k) const {
    return arr[k];
  }
};

class test {
public:
  int i;

  test(int _i) : i(_i) { }

  std::tuple<int> produce(void) {
    return std::forward_as_tuple(i);
  }

  void consume(int j) {
    std::cout << i << " received a message from " << j << std::endl;
  }
};

int main(void) {
  const int n = 10;
  std::vector<test> _arr;
  // producer and consumer indices
  std::vector<int> psi(n);
  std::vector<int> csi(n);
  // initialize psi, csi and arr
  std::iota(psi.begin(), psi.end(), 0);
  std::transform(psi.begin(), psi.end(), csi.begin(), [n] (int i) {
    return (i + 1) % n;
  });
  std::for_each(psi.begin(), psi.end(), [&_arr](int i) mutable {
    _arr.emplace_back(i);
  });
  // wrap the vector in a orch friendly wrapper
  test_array<int, test> arr(_arr);
  // index into the array to generate the producers and consumers
  // using psi as the index set for the producers with test::produce as the producing method
  auto ps = orcha::pmap(arr, psi, orcha::bind(&test::produce));
  // and csi as the index set for the consumers with test::consume as the consuming method
  auto cs = orcha::cmap(arr, csi, orcha::bind(&test::consume));
  // then map the producers onto the consumers
  orcha::produce(ps, cs);
  // then reduce on the producers
  // (an explicit cast is currently required for std::functional built-ins)
  auto plus = static_cast<std::function<int(int, int)>>(std::plus<int>());
  std::cout << "reduction evaluated to " << orcha::reduce(ps, 0, plus) << std::endl;
}
