#pragma once

#include <type_traits>
#include <cstdint>
#include <tuple>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>
#include <stdexcept>

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

  template<typename T, typename K, typename V, typename R, typename... As>
  std::vector<std::function<R(As...)>> map(T &t, std::vector<K> ks, std::function<std::function<R(As...)>(V&)> f);

  template<typename T, typename R = void>
  inline void strate(const std::vector<std::function<T(void)>> &ps, const std::vector<std::function<void(T)>> &cs);

  template<typename T, typename R, typename std::enable_if<!std::is_void<R>::value>::type>
  inline std::vector<std::function<R(void)>> strate(const std::vector<std::function<T(void)>> &ps, const std::vector<std::function<R(T)>> &cs);

  template<typename... Ts, typename R = void>
  inline void strate(const std::vector<std::function<std::tuple<Ts...>(void)>> &ps, const std::vector<std::function<void(Ts...)>> &cs);

  // TODO Add non-void strate definition for Ts.../R

  template<typename A, typename B>
  std::vector<std::function<std::tuple<A, B>(void)>> zip2(std::vector<std::function<A(void)>> as, std::vector<std::function<B(void)>> bs);

  template<typename A, typename B>
  std::vector<std::function<void(std::tuple<A, B>)>> zip2(std::vector<std::function<void(A)>> as, std::vector<std::function<void(B)>> bs);

  // template<typename A, typename B, typename C>
  // std::vector<std::function<std::tuple<A, B, C>(void)>> zip3(std::vector<std::function<A(void)>> as, std::vector<std::function<B(void)>> bs, std::vector<std::function<C(void)>> cs);

  template<typename T>
  void consume(std::vector<std::function<void(T)>> consumers, std::vector<T> values);

  template<typename... Ts>
  void consume(std::vector<std::function<void(Ts...)>> consumers, std::vector<std::tuple<Ts...>> values);

  template<typename T>
  std::vector<T> produce(std::vector<std::function<T(void)>> ps);

  template<typename T, typename R>
  R reduce(std::vector<std::function<T(void)>> ps, R init, std::function<R(R,T)> reducer);

  template<typename T, typename... Ts>
  T reduce(std::vector<std::function<std::tuple<Ts...>(void)>> producers, T init, std::function<T(T,Ts...)> reducer);

  template<typename R, template<typename...> class Ps, typename... As, int... Is>
  R call(std::function<R(As...)> const &f, Ps<As...> const &ps, int_sequence<Is...>);

  template<typename R, template<typename...> class Ps, typename... As>
  R call(std::function<R(As...)> const &f, Ps<As...> const &ps);
};

#include "orcha.tcc"
