#pragma once

template<class T, class R, class... As, int... Is>
std::function<R(As...)> orcha::bind(R(T::*p)(As...), T &t, int_sequence<Is...>) {
  return std::bind(p, t, placeholder_template<Is>{}...);
}

template<class T, class R, class... As>
std::function<R(As...)> orcha::bind(R(T::*p)(As...), T &t) {
  return orcha::bind(p, t, make_int_sequence<sizeof...(As)>{});
}

template<class T, class R, class... As>
std::function<std::function<R(As...)>(T&)> orcha::bind(R(T::*p)(As...)) {
  return [p](T &t) {
    return orcha::bind(p, t, make_int_sequence<sizeof...(As)>{});
  };
}

template<typename T, typename K, typename V, typename R, typename... As>
std::vector<std::function<R(As...)>> orcha::map(T &t, std::vector<K> ks, std::function<std::function<R(As...)>(V&)> f) {
  std::vector<std::function<R(As...)>> fs;

  std::transform(ks.begin(), ks.end(), std::back_inserter(fs), [t, f] (K k) {
    // TODO overload map with a const and non-const version
    return f(const_cast<V&>(t[k]));
  });

  return fs;
}

template<typename T, typename R = void>
inline void orcha::strate(const std::vector<std::function<T(void)>> &ps, const std::vector<std::function<void(T)>> &cs) {
  if (ps.size() != cs.size()) {
    throw std::runtime_error("orchestrate: number of objects must match!");
  }

  for (auto i = ps.begin(), j = cs.begin(); i < ps.end() && j < cs.end(); i++, j++) {
    // ckschedule(..., [i, j]() {
      (*j)((*i)());
    // }, ...);
  }
}

template<typename T, typename R, typename std::enable_if<!std::is_void<R>::value>::type>
inline std::vector<std::function<R(void)>> orcha::strate(const std::vector<std::function<T(void)>> &ps, const std::vector<std::function<R(T)>> &cs) {
  if (ps.size() != cs.size()) {
    throw std::runtime_error("orchestrate: number of objects must match!");
  }

  std::vector<std::function<R(void)>> rs;

  for (auto p = ps.begin(), c = cs.begin(); p < ps.end() && c < cs.end(); p++, c++) {
    rs.push_back([p, c] (void) {
      // ckschedule(..., [i, j]() {
        return (*c)((*p)());
      // }, ...);
    });
  }

  return rs;
}

template<typename... Ts, typename R = void>
inline void orcha::strate(const std::vector<std::function<std::tuple<Ts...>(void)>> &ps, const std::vector<std::function<void(Ts...)>> &cs) {
  if (ps.size() != cs.size()) {
    throw std::runtime_error("orchestrate: number of objects must match!");
  }

  for (auto p = ps.begin(), c = cs.begin(); p < ps.end() && c < cs.end(); p++, c++) {
    // ckschedule(..., [i, j]() {
      orcha::call(*c, (*p)());
    // }, ...);
  }
}

template<typename A, typename B>
std::vector<std::function<std::tuple<A, B>(void)>> orcha::zip2(std::vector<std::function<A(void)>> as, std::vector<std::function<B(void)>> bs) {
  if (as.size() != bs.size()) {
    throw std::runtime_error("zip: number of objects must match!");
  }

  std::vector<std::function<std::tuple<A, B>(void)>> cs;

  for (auto a = as.begin(), b = bs.begin(); a < as.end() && b < bs.end(); a++, b++) {
    cs.push_back([a, b] (void) {
      return std::make_tuple((*a)(), (*b)());
    });
  }

  return cs;
}

template<typename T>
std::vector<T> orcha::produce(std::vector<std::function<T(void)>> ps) {
  std::vector<T> vs;

  std::transform(ps.begin(), ps.end(), std::back_inserter(vs), [](std::function<T(void)> p) {
    return p();
  });

  return vs;
}

template<typename T>
void orcha::consume(std::vector<std::function<void(T)>> consumers, std::vector<T> values) {
  if (values.size() != consumers.size()) {
    throw std::runtime_error("consume: number of objects must match!");
  }

  for (auto i = values.begin(), j = consumers.begin(); i < values.end() && j < consumers.end(); i++, j++) {
    (*j)(*i);
  }
}

template<typename... Ts>
void orcha::consume(std::vector<std::function<void(Ts...)>> consumers, std::vector<std::tuple<Ts...>> values) {
  if (values.size() != consumers.size()) {
    throw std::runtime_error("consume: number of objects must match!");
  }

  for (auto i = values.begin(), j = consumers.begin(); i < values.end() && j < consumers.end(); i++, j++) {
    orcha::call(*j, *i);
  }
}

template<typename R, template<typename...> class Ps, typename... As, int... Is>
R orcha::call(std::function<R(As...)> const &f, Ps<As...> const &ps, int_sequence<Is...>) {
  return f(std::get<Is>(ps)...);
}

template<typename R, template<typename...> class Ps, typename... As>
R orcha::call(std::function<R(As...)> const &f, Ps<As...> const &ps) {
  return call(f, ps, make_int_sequence<sizeof...(As)>{});
}

template<typename T, typename R>
R orcha::reduce(std::vector<std::function<T(void)>> ps, R init, std::function<R(R,T)> reducer) {
  auto vs = orcha::produce(ps);
  // on C++17 or newer use parallel reduce
  return std::accumulate(vs.begin(), vs.end(), init, reducer);
}

template<typename T, typename... Ts>
T orcha::reduce(std::vector<std::function<std::tuple<Ts...>(void)>> producers, T init, std::function<T(T,Ts...)> reducer) {
  auto values = orcha::produce(producers);
  // on C++17 or newer use parallel reduce
  return std::accumulate(values.begin(), values.end(), init, [reducer](T t, std::tuple<Ts...> ts) {
    std::tuple<T, Ts...> args = std::tuple_cat(std::forward_as_tuple(t), ts);
    return orcha::call(reducer, args);
  });
}
