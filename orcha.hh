#include <type_traits>
#include <cstdint>
#include <tuple>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>
#include <stdexcept>
#pragma once

template<int...> struct int_sequence {};

template<int N, int... Is> struct make_int_sequence
    : make_int_sequence<N-1, N-1, Is...> {};
template<int... Is> struct make_int_sequence<0, Is...>
    : int_sequence<Is...> {};

template<int> // begin with 0 here!
struct placeholder_template
{};

namespace std
{
    template<int N>
    struct is_placeholder< placeholder_template<N> >
        : integral_constant<int, N+1> // the one is important
    {};
};

namespace orcha
{
  template<class T, class R, class... As, int... Is>
  std::function<R(As...)> bind(R(T::*p)(As...), T &t, int_sequence<Is...>);

  template<class T, class R, class... As>
  std::function<R(As...)> bind(R(T::*p)(As...), T &t);

  template<class T, class R, class... As>
  std::function<std::function<R(As...)>(T&)> bind(R(T::*p)(As...));

  template<typename K, typename V, template <class> typename F, template<typename, typename> class T, typename... Ts>
  std::vector<std::function<std::tuple<Ts...>(void)>> pmap(T<K, V> &p, std::vector<K> ks, F<std::function<std::tuple<Ts...>(void)>(V&)> f);

  template<typename K, typename V, template <class> typename F, template<typename, typename> class T, typename... Ts>
  std::vector<std::function<void(Ts...)>> cmap(T<K, V> &c, std::vector<K> ks, F<std::function<void(Ts...)>(V&)> f);

  template<typename... Ts>
  void produce(std::vector<std::function<std::tuple<Ts...>(void)>> producers, std::vector<std::function<void(Ts...)>> consumers);

  template<typename... Ts>
  std::vector<std::tuple<Ts...>> produce(std::vector<std::function<std::tuple<Ts...>(void)>> producers);

  template<typename... Ts>
  inline void consume(std::vector<std::function<void(Ts...)>> consumers, std::vector<std::function<std::tuple<Ts...>(void)>> producers);

  template<typename... Ts>
  void consume(std::vector<std::function<void(Ts...)>> consumers, std::vector<std::tuple<Ts...>> values);

  template<typename T, typename... Ts>
  T reduce(std::vector<std::function<std::tuple<Ts...>(void)>> producers, T init, std::function<T(T,Ts...)> reducer);

  template<typename R, template<typename...> class Ps, typename... As, int... Is>
  R call(std::function<R(As...)> const &f, Ps<As...> const &ps, int_sequence<Is...>);

  template<typename R, template<typename...> class Ps, typename... As>
  R call(std::function<R(As...)> const &f, Ps<As...> const &ps);
};

#include <orcha.tcc>
