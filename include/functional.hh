#pragma once

#include <functional>

template<int...> struct int_sequence {};

template<int N, int... Is> struct make_int_sequence
    : make_int_sequence<N-1, N-1, Is...> {};
template<int... Is> struct make_int_sequence<0, Is...>
    : int_sequence<Is...> {};

template<int> struct placeholder_template {}; // begin with 0 here!

namespace std {
    template<int N>
    struct is_placeholder< placeholder_template<N> >
        : integral_constant<int, N+1> // the one is important
    {};
};

namespace orcha {
  namespace functional {
    template<class T, class R, class... As, int... Is>
    std::function<R(As...)> bind(R(T::*p)(As...), T *t, int_sequence<Is...>);

    template<class T, class R, class... As>
    std::function<R(As...)> bind(R(T::*p)(As...), T *t);

    template<class T, class R, class... As>
    std::function<std::function<R(As...)>(T*)> bind(R(T::*p)(As...));

    template<typename R, template<typename...> class Ps, typename... As, int... Is>
    R call(std::function<R(As...)> const &f, Ps<As...> const &ps, int_sequence<Is...>);

    template<typename R, template<typename...> class Ps, typename... As>
    R call(std::function<R(As...)> const &f, Ps<As...> const &ps);
  }
};

template<class T, class R, class... As, int... Is>
std::function<R(As...)> orcha::functional::bind(R(T::*p)(As...), T *t, int_sequence<Is...>) {
  return std::bind(p, t, placeholder_template<Is>{}...);
}

template<class T, class R, class... As>
std::function<R(As...)> orcha::functional::bind(R(T::*p)(As...), T *t) {
  return orcha::functional::bind(p, t, make_int_sequence<sizeof...(As)>{});
}

template<class T, class R, class... As>
std::function<std::function<R(As...)>(T*)> orcha::functional::bind(R(T::*p)(As...)) {
  return [p](T* t) {
    return orcha::functional::bind(p, t, make_int_sequence<sizeof...(As)>{});
  };
}

template<typename R, template<typename...> class Ps, typename... As, int... Is>
R orcha::functional::call(std::function<R(As...)> const &f, Ps<As...> const &ps, int_sequence<Is...>) {
  return f(std::get<Is>(ps)...);
}

template<typename R, template<typename...> class Ps, typename... As>
R orcha::functional::call(std::function<R(As...)> const &f, Ps<As...> const &ps) {
  return call(f, ps, make_int_sequence<sizeof...(As)>{});
}
