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

template<typename K, typename V, template <class> typename F, template<typename, typename> class T, typename... Ts>
std::vector<std::function<std::tuple<Ts...>(void)>> orcha::pmap(T<K, V> &p, std::vector<K> ks, F<std::function<std::tuple<Ts...>(void)>(V&)> f) {
  std::vector<std::function<std::tuple<Ts...>(void)>> ps;

  std::transform(ks.begin(), ks.end(), std::back_inserter(ps), [p, f] (K k) {
    return f(p(k));
  });

  return ps;
}

template<typename K, typename V, template <class> typename F, template<typename, typename> class T, typename... Ts>
std::vector<std::function<void(Ts...)>> orcha::cmap(T<K, V> &c, std::vector<K> ks, F<std::function<void(Ts...)>(V&)> f) {
  std::vector<std::function<void(Ts...)>> cs;

  std::transform(ks.begin(), ks.end(), std::back_inserter(cs), [c, f] (K k) {
    return f(c(k));
  });

  return cs;
}

template<typename... Ts>
void orcha::produce(std::vector<std::function<std::tuple<Ts...>(void)>> producers, std::vector<std::function<void(Ts...)>> consumers) {
  if (producers.size() != consumers.size()) {
    throw std::runtime_error("number of producers must match number of consumers!");
  }

  for (auto i = producers.begin(), j = consumers.begin(); i < producers.end() && j < consumers.end(); i++, j++) {
    orcha::call(*j, (*i)());
  }
}

template<typename... Ts>
inline void orcha::consume(std::vector<std::function<void(Ts...)>> consumers, std::vector<std::function<std::tuple<Ts...>(void)>> producers) {
  orcha::produce(producers, consumers);
}

template<typename... Ts>
std::vector<std::tuple<Ts...>> orcha::produce(std::vector<std::function<std::tuple<Ts...>(void)>> producers) {
  std::vector<std::tuple<Ts...>> values;

  for (auto i = producers.begin(); i < producers.end(); i++) {
    values.push_back((*i)());
  }

  return values;
}

template<typename... Ts>
void orcha::consume(std::vector<std::function<void(Ts...)>> consumers, std::vector<std::tuple<Ts...>> values) {
  if (values.size() != consumers.size()) {
    throw std::runtime_error("number of values must match number of consumers!");
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

template<typename T, typename... Ts>
T orcha::reduce(std::vector<std::function<std::tuple<Ts...>(void)>> producers, T init, std::function<T(T,Ts...)> reducer) {
  auto values = orcha::produce(producers);
  // on C++17 or newer use parallel reduce
  return std::accumulate(values.begin(), values.end(), init, [reducer](T t, std::tuple<Ts...> ts) {
    std::tuple<T, Ts...> args = std::tuple_cat(std::forward_as_tuple(t), ts);
    return orcha::call(reducer, args);
  });
}
