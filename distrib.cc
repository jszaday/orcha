#include "distrib.hh"

namespace orcha {
  std::vector<arr_t> k_arrs_;
  std::vector<reg_t> k_funs_;
}

template<typename V, typename K, typename S, typename... As>
std::shared_ptr<orcha::DistributedArray<K, V>> orcha::distribute(S strategy, const orcha::IndexSpace<K> &is, As... args) {
  std::shared_ptr<DistributedArray<K, V>> arr = std::make_shared<DistributedArray<K, V>>(k_arrs_.size(), &strategy, is, &args...);
  k_arrs_.push_back(arr);
  return std::move(arr);
}

template<typename K, typename V, typename R, typename... As>
orcha::RegisteredFunction<K, V, R, As...> orcha::register_function(std::shared_ptr<orcha::DistributedArray<K, V>> arr, R(V::*p)(As...)) {
  reg_t f = [arr, p] (id_t i) {
    return archive::wrap(functional::bind(p, (*arr)[arr->key_for(i)]));
  };
  k_funs_.push_back(std::move(f));
  return RegisteredFunction<K, V, R, As...>(k_funs_.size() - 1, arr);
}
