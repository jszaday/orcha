#pragma once

#include <functional>
#include <string>
#include <sstream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <serialize-tuple/serialize_tuple.h>
#include <boost/serialization/vector.hpp>

namespace orcha {
  namespace archive {
    using istream_t  = std::istringstream;
    using ostream_t  = std::ostringstream;
    using iarchive_t = boost::archive::binary_iarchive;
    using oarchive_t = boost::archive::binary_oarchive;
    using function_t = std::function<std::string(const std::string&)>;

    template<typename R, typename A>
    function_t wrap(std::function<R(A)> f) {
      return [f](const std::string& s) {
        istream_t is(s); ostream_t os;
        iarchive_t ia(is); oarchive_t oa(os);
        A a; ia >> a; oa << f(a);
        return os.str();
      };
    }

// uses _ as a pretty placeholder
#define _ std::string()

    template<typename A>
    function_t wrap(std::function<void(A)> f) {
      return [f](const std::string& s) {
        istream_t is(s);
        iarchive_t ia(is);
        A a; ia >> a; f(a);
        return _;
      };
    }

#undef  _
#define _

    template<typename R>
    function_t wrap(std::function<R(void)> f) {
      return [f](const std::string& s) {
        ostream_t os;
        oarchive_t oa(os);
        oa << f(_);
        return os.str();
      };
    }

#undef _
  }
}
